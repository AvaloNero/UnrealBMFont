# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$EngineRoot,

	[string]$OutputDirectory,

	[ValidateNotNullOrEmpty()]
	[string[]]$TargetPlatforms = @('Win64'),

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
	$parentPrefix = [System.IO.Path]::TrimEndingDirectorySeparator($fullParent) + [System.IO.Path]::DirectorySeparatorChar
	return $fullCandidate.StartsWith($parentPrefix, [System.StringComparison]::OrdinalIgnoreCase)
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

$stagingBase = Join-Path $temporaryRoot 'Staging'
$stagingRoot = Join-Path $stagingBase ([System.Guid]::NewGuid().ToString('N'))
$stagedPluginRoot = Join-Path $stagingRoot 'UnrealBMFont'
$stagedPlugin = Join-Path $stagedPluginRoot 'UnrealBMFont.uplugin'

try {
	New-Item -ItemType Directory -Path $stagedPluginRoot -Force | Out-Null
	$excludedDirectories = @(
		'.git',
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
		"-Package=$fullOutputDirectory" `
		"-TargetPlatforms=$platformArgument" `
		-Rocket

	if ($LASTEXITCODE -ne 0) {
		throw "BuildPlugin failed with exit code $LASTEXITCODE. Output remains at $fullOutputDirectory for inspection."
	}
} finally {
	if (Test-Path -LiteralPath $stagingRoot) {
		if (-not (Test-IsPathInsideDirectory -Candidate $stagingRoot -Parent $stagingBase)) {
			throw "Refusing to remove an unexpected staging path: $stagingRoot"
		}
		Remove-Item -LiteralPath $stagingRoot -Recurse -Force
	}
}

# BuildPlugin intentionally removes EnabledByDefault while marking the package as
# installed. Restoring the explicit opt-in prevents a project-local precompiled
# package from being mistaken for part of the generic UnrealGame target.
$packagedDescriptor = Join-Path $fullOutputDirectory 'UnrealBMFont.uplugin'
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
	$pdbFiles = @(Get-ChildItem -LiteralPath $fullOutputDirectory -Recurse -Filter '*.pdb' -File)
	$pdbBytes = ($pdbFiles | Measure-Object -Property Length -Sum).Sum
	foreach ($pdbFile in $pdbFiles) {
		Remove-Item -LiteralPath $pdbFile.FullName -Force
	}
	if ($pdbFiles.Count -gt 0) {
		Write-Host ("Stripped {0} debug symbol file(s), {1:N1} MB, from the package." -f $pdbFiles.Count, ($pdbBytes / 1MB))
	}
}

Write-Host "Unreal BMFont package created at: $fullOutputDirectory"
