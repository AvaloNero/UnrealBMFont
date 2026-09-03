# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$EngineRoot,

	[string]$OutputDirectory,

	[ValidateNotNullOrEmpty()]
	[string[]]$TargetPlatforms = @('Win64'),

	[string]$BuildRoot,

	[switch]$KeepPdb
)

$ErrorActionPreference = 'Stop'

function Test-IsPathInsideDirectory {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Candidate,

		[Parameter(Mandatory = $true)]
		[string]$Parent
	)

	$fullCandidate = [System.IO.Path]::GetFullPath($Candidate)
	$fullParent = [System.IO.Path]::GetFullPath($Parent)
	if ($fullCandidate.Equals($fullParent, [System.StringComparison]::OrdinalIgnoreCase)) {
		return $true
	}
	$parentPrefix = [System.IO.Path]::TrimEndingDirectorySeparator($fullParent) + [System.IO.Path]::DirectorySeparatorChar
	return $fullCandidate.StartsWith($parentPrefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Test-FileContainsText {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string[]]$Needles
	)

	$latin1 = [System.Text.Encoding]::GetEncoding(28591)
	$utf8 = [System.Text.UTF8Encoding]::new($false)
	$utf16 = [System.Text.Encoding]::Unicode
	$patterns = @()
	foreach ($needle in $Needles) {
		foreach ($variant in @($needle, $needle.Replace('\', '/'))) {
			$patterns += $latin1.GetString($utf8.GetBytes($variant))
			$patterns += $latin1.GetString($utf16.GetBytes($variant))
		}
	}
	$patterns = @($patterns | Where-Object { $_.Length -gt 0 } | Select-Object -Unique)
	if ($patterns.Count -eq 0) {
		return $false
	}

	$maxPatternLength = ($patterns | Measure-Object -Property Length -Maximum).Maximum
	$chunkSize = 1MB
	$buffer = [byte[]]::new($chunkSize + $maxPatternLength - 1)
	$carryLength = 0
	$stream = [System.IO.File]::OpenRead($Path)
	try {
		while (($readLength = $stream.Read($buffer, $carryLength, $chunkSize)) -gt 0) {
			$totalLength = $carryLength + $readLength
			$byteString = $latin1.GetString($buffer, 0, $totalLength)
			foreach ($pattern in $patterns) {
				if ($byteString.IndexOf($pattern, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
					return $true
				}
			}

			$carryLength = [System.Math]::Min($maxPatternLength - 1, $totalLength)
			if ($carryLength -gt 0) {
				[System.Buffer]::BlockCopy(
					$buffer,
					$totalLength - $carryLength,
					$buffer,
					0,
					$carryLength
				)
			}
		}
		return $false
	} finally {
		$stream.Dispose()
	}
}

$resolvedEngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$uat = Join-Path $resolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $uat -PathType Leaf)) {
	throw "RunUAT.bat was not found under EngineRoot: $resolvedEngineRoot"
}

$pluginRoot = Split-Path -Parent $PSScriptRoot
$sourcePlugin = Join-Path $pluginRoot 'UnrealBMFont.uplugin'
if (-not (Test-Path -LiteralPath $sourcePlugin -PathType Leaf)) {
	throw "Plugin descriptor was not found: $sourcePlugin"
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) 'UnrealBMFont'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
	$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
	$OutputDirectory = Join-Path $temporaryRoot "Artifacts\UnrealBMFont-$stamp"
}
$fullOutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (Test-IsPathInsideDirectory -Candidate $fullOutputDirectory -Parent $pluginRoot) {
	throw "OutputDirectory must be outside the plugin source tree so later builds cannot stage old packages: $fullOutputDirectory"
}
if (Test-Path -LiteralPath $fullOutputDirectory) {
	$existingItems = @(Get-ChildItem -LiteralPath $fullOutputDirectory -Force)
	if ($existingItems.Count -gt 0) {
		throw "OutputDirectory must be empty to prevent accidental overwrite: $fullOutputDirectory"
	}
} else {
	New-Item -ItemType Directory -Path $fullOutputDirectory | Out-Null
}

$commonDocuments = [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::CommonDocuments)
if ([string]::IsNullOrWhiteSpace($BuildRoot)) {
	if ([string]::IsNullOrWhiteSpace($commonDocuments)) {
		throw 'The shared Documents directory could not be resolved; pass -BuildRoot with a neutral path outside the plugin tree.'
	}
	$BuildRoot = Join-Path $commonDocuments 'UnrealBMFontBuild'
}
$fullBuildRoot = [System.IO.Path]::GetFullPath($BuildRoot)
if (Test-IsPathInsideDirectory -Candidate $fullBuildRoot -Parent $pluginRoot) {
	throw "BuildRoot must be outside the plugin source tree: $fullBuildRoot"
}
if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE) `
	-and (Test-IsPathInsideDirectory -Candidate $fullBuildRoot -Parent $env:USERPROFILE)) {
	throw "BuildRoot must not be inside the current user profile because compile paths enter release binaries: $fullBuildRoot"
}

# BuildPlugin embeds compile paths in binaries and intermediate objects. Keep its host,
# source staging, and package output under a shared neutral root, then copy only the
# audited result to the caller's requested destination.
$buildRunRoot = Join-Path $fullBuildRoot ("Run-" + [System.Guid]::NewGuid().ToString('N'))
$stagingRoot = Join-Path $buildRunRoot 'Source'
$stagedPluginRoot = Join-Path $stagingRoot 'UnrealBMFont'
$stagedPlugin = Join-Path $stagedPluginRoot 'UnrealBMFont.uplugin'
$internalPackageDirectory = Join-Path $buildRunRoot 'Package'

try {
	New-Item -ItemType Directory -Path $stagedPluginRoot -Force | Out-Null
	$excludedDirectories = @(
		'.git',
		'.codegraph',
		'.build',
		'Artifacts',
		'Binaries',
		'DerivedDataCache',
		'Intermediate',
		'Saved',
		'.vs',
		'.idea',
		'.vscode',
		'__pycache__'
	) | ForEach-Object { Join-Path $pluginRoot $_ }
	$robocopyArguments = @(
		$pluginRoot,
		$stagedPluginRoot,
		'/E',
		'/R:1',
		'/W:1',
		'/COPY:DAT',
		'/DCOPY:DAT',
		'/NFL',
		'/NDL',
		'/NJH',
		'/NJS',
		'/NP',
		'/XD'
	) + $excludedDirectories + @('/XF', '*.pyc')
	& robocopy.exe @robocopyArguments
	$stagingExitCode = $LASTEXITCODE
	if ($stagingExitCode -gt 7) {
		throw "Failed to create a clean plugin staging copy; robocopy exited with code $stagingExitCode."
	}
	if (-not (Test-Path -LiteralPath $stagedPlugin -PathType Leaf)) {
		throw "The staged plugin descriptor was not created: $stagedPlugin"
	}

	$stagedFileCount = @(Get-ChildItem -LiteralPath $stagedPluginRoot -Recurse -File -Force).Count
	Write-Host "Staged $stagedFileCount source file(s) without local build products: $stagedPluginRoot"

	$platformArgument = $TargetPlatforms -join '+'
	& $uat BuildPlugin `
		"-Plugin=$stagedPlugin" `
		"-Package=$internalPackageDirectory" `
		"-TargetPlatforms=$platformArgument" `
		-WaitForUATMutex `
		-Rocket

	if ($LASTEXITCODE -ne 0) {
		throw "BuildPlugin failed with exit code $LASTEXITCODE."
	}

	# BuildPlugin intentionally removes EnabledByDefault while marking the package as
	# installed. Restoring the explicit opt-in prevents a project-local precompiled
	# package from being mistaken for part of the generic UnrealGame target.
	$packagedDescriptor = Join-Path $internalPackageDirectory 'UnrealBMFont.uplugin'
	if (-not (Test-Path -LiteralPath $packagedDescriptor -PathType Leaf)) {
		throw "BuildPlugin succeeded but did not produce the plugin descriptor: $packagedDescriptor"
	}
	$descriptor = Get-Content -LiteralPath $packagedDescriptor -Raw | ConvertFrom-Json
	$enabledByDefault = $descriptor.PSObject.Properties['EnabledByDefault']
	if ($null -eq $enabledByDefault) {
		$descriptor | Add-Member -NotePropertyName 'EnabledByDefault' -NotePropertyValue $false
	} else {
		$enabledByDefault.Value = $false
	}
	$descriptorJson = $descriptor | ConvertTo-Json -Depth 100
	$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
	[System.IO.File]::WriteAllText($packagedDescriptor, $descriptorJson + [Environment]::NewLine, $utf8WithoutBom)

	$verifiedDescriptor = Get-Content -LiteralPath $packagedDescriptor -Raw | ConvertFrom-Json
	if ($verifiedDescriptor.EnabledByDefault -ne $false -or $verifiedDescriptor.Installed -ne $true) {
		throw "Packaged descriptor invariants were not preserved: $packagedDescriptor"
	}

	# Debug symbols are not part of the distributable package: the three editor PDBs
	# account for the overwhelming majority of the UAT output. The Intermediate .obj/.lib
	# files stay because precompiled game-target linking needs them.
	if (-not $KeepPdb) {
		$pdbFiles = @(Get-ChildItem -LiteralPath $internalPackageDirectory -Recurse -Filter '*.pdb' -File)
		$pdbBytes = ($pdbFiles | Measure-Object -Property Length -Sum).Sum
		foreach ($pdbFile in $pdbFiles) {
			Remove-Item -LiteralPath $pdbFile.FullName -Force
		}
		if ($pdbFiles.Count -gt 0) {
			Write-Host ("Stripped {0} debug symbol file(s), {1:N1} MB, from the package." -f $pdbFiles.Count, ($pdbBytes / 1MB))
		}
	}

	$requiredPackageFiles = @(
		'LICENSE',
		'THIRD_PARTY_NOTICES.md',
		'README.md',
		'README.zh-CN.md',
		'CHANGELOG.md',
		'CODE_OF_CONDUCT.md',
		'CONTRIBUTING.md',
		'ROADMAP.md',
		'SECURITY.md',
		'SUPPORT.md',
		'Docs\Architecture.md',
		'Docs\Compatibility.md',
		'Docs\FormatSupport.md',
		'Docs\ReleaseChecklist.md',
		'Docs\RichText.md',
		'Docs\Testing.md',
		'Docs\Images\showcase-runtime.png',
		'Samples\Minimal\Minimal.fnt',
		'Samples\Minimal\Minimal.png',
		'Samples\Packed\Packed.fnt',
		'Samples\Packed\Packed.png',
		'Samples\Showcase\Showcase.fnt',
		'Samples\Showcase\Showcase.png'
	)
	foreach ($relativePath in $requiredPackageFiles) {
		$requiredPath = Join-Path $internalPackageDirectory $relativePath
		if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
			throw "Required release file is missing from the package: $relativePath"
		}
	}
	foreach ($forbiddenEntry in @('Scripts', '.build', '.codegraph', 'Artifacts')) {
		$forbiddenPath = Join-Path $internalPackageDirectory $forbiddenEntry
		if (Test-Path -LiteralPath $forbiddenPath) {
			throw "Repository-only entry leaked into the release package: $forbiddenEntry"
		}
	}

	$personalRoots = @($env:USERPROFILE, $pluginRoot) `
		| Where-Object { -not [string]::IsNullOrWhiteSpace($_) } `
		| ForEach-Object { [System.IO.Path]::GetFullPath($_) } `
		| Select-Object -Unique
	$pathLeaks = @(
		Get-ChildItem -LiteralPath $internalPackageDirectory -Recurse -File -Force | Where-Object {
			Test-FileContainsText -Path $_.FullName -Needles $personalRoots
		} | ForEach-Object {
			[System.IO.Path]::GetRelativePath($internalPackageDirectory, $_.FullName)
		}
	)
	if ($pathLeaks.Count -gt 0) {
		throw "Package contains personal/source absolute paths in: $($pathLeaks -join ', ')"
	}

	$copyArguments = @(
		$internalPackageDirectory,
		$fullOutputDirectory,
		'/E',
		'/R:1',
		'/W:1',
		'/COPY:DAT',
		'/DCOPY:DAT',
		'/NFL',
		'/NDL',
		'/NJH',
		'/NJS',
		'/NP'
	)
	& robocopy.exe @copyArguments
	$copyExitCode = $LASTEXITCODE
	if ($copyExitCode -gt 7) {
		throw "Failed to copy the audited plugin package; robocopy exited with code $copyExitCode."
	}
	# Robocopy uses 1-7 for successful copy variants. Do not leak that native status
	# to callers that inspect LASTEXITCODE after this script returns.
	$global:LASTEXITCODE = 0

	Write-Host "Package audit passed: release files present and no personal/source paths found."
	Write-Host "Unreal BMFont package created at: $fullOutputDirectory"
} finally {
	if (Test-Path -LiteralPath $buildRunRoot) {
		if (-not (Test-IsPathInsideDirectory -Candidate $buildRunRoot -Parent $fullBuildRoot)) {
			throw "Refusing to remove an unexpected build path: $buildRunRoot"
		}
		Remove-Item -LiteralPath $buildRunRoot -Recurse -Force
	}
}
