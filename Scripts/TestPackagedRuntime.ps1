# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$EngineRoot,

	[Parameter(Mandatory = $true)]
	[string]$PluginPackage,

	[string]$WorkDirectory,

	[ValidateSet('Development', 'Shipping')]
	[string[]]$Configurations = @('Development', 'Shipping'),

	[ValidateSet('Win64')]
	[string]$TargetPlatform = 'Win64',

	[ValidateRange(10, 600)]
	[int]$LaunchTimeoutSeconds = 90
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

function Write-Utf8File {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string]$Content
	)

	$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
	[System.IO.File]::WriteAllText($Path, $Content, $utf8WithoutBom)
}

$resolvedEngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$uat = Join-Path $resolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $uat -PathType Leaf)) {
	throw "RunUAT.bat was not found under EngineRoot: $resolvedEngineRoot"
}

$engineVersionFile = Join-Path $resolvedEngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path -LiteralPath $engineVersionFile -PathType Leaf)) {
	throw "Engine Build.version was not found: $engineVersionFile"
}
$engineVersion = Get-Content -LiteralPath $engineVersionFile -Raw | ConvertFrom-Json
if ([int]$engineVersion.MajorVersion -ne 5 -or [int]$engineVersion.MinorVersion -ne 8) {
	throw "This release smoke host is currently verified for Unreal Engine 5.8; found $($engineVersion.MajorVersion).$($engineVersion.MinorVersion)."
}

$resolvedPluginPackage = (Resolve-Path -LiteralPath $PluginPackage).Path
$packagedDescriptor = Join-Path $resolvedPluginPackage 'UnrealBMFont.uplugin'
if (-not (Test-Path -LiteralPath $packagedDescriptor -PathType Leaf)) {
	throw "PluginPackage must contain UnrealBMFont.uplugin: $resolvedPluginPackage"
}
$descriptor = Get-Content -LiteralPath $packagedDescriptor -Raw | ConvertFrom-Json
if ($descriptor.Installed -ne $true -or $descriptor.EnabledByDefault -ne $false) {
	throw "PluginPackage must be an installed, explicitly opt-in BuildPlugin package: $packagedDescriptor"
}
if ([string]::IsNullOrWhiteSpace([string]$descriptor.VersionName)) {
	throw "PluginPackage has no VersionName: $packagedDescriptor"
}

$pluginRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($WorkDirectory)) {
	$WorkDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
		'UnrealBMFont\RuntimeSmoke\Run-' + [System.Guid]::NewGuid().ToString('N')
	)
}
$fullWorkDirectory = [System.IO.Path]::GetFullPath($WorkDirectory)
if (Test-IsPathInsideDirectory -Candidate $fullWorkDirectory -Parent $pluginRoot) {
	throw "WorkDirectory must be outside the plugin source tree: $fullWorkDirectory"
}
if (Test-Path -LiteralPath $fullWorkDirectory) {
	throw "WorkDirectory must not already exist, preventing stale cook or runtime evidence: $fullWorkDirectory"
}

$hostRoot = Join-Path $fullWorkDirectory 'RuntimeHost'
$hostPluginDirectory = Join-Path $hostRoot 'Plugins\UnrealBMFont'
$hostSourceDirectory = Join-Path $hostRoot 'Source\RuntimeHost'
$hostConfigDirectory = Join-Path $hostRoot 'Config'
$evidenceDirectory = Join-Path $fullWorkDirectory 'Evidence'
New-Item -ItemType Directory -Path $hostPluginDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $hostSourceDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $hostConfigDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDirectory -Force | Out-Null
Get-ChildItem -LiteralPath $resolvedPluginPackage -Force | ForEach-Object {
	Copy-Item -LiteralPath $_.FullName -Destination $hostPluginDirectory -Recurse -Force
}

$projectFile = Join-Path $hostRoot 'RuntimeHost.uproject'
$projectDescriptor = [ordered]@{
	FileVersion = 3
	DisableEnginePluginsByDefault = $true
	Modules = @(
		[ordered]@{
			Name = 'RuntimeHost'
			Type = 'Runtime'
			LoadingPhase = 'Default'
			AdditionalDependencies = @('Engine', 'UMG', 'UnrealBMFont')
		}
	)
	Plugins = @(
		[ordered]@{
			Name = 'UnrealBMFont'
			Enabled = $true
		}
	)
}
Write-Utf8File -Path $projectFile -Content (($projectDescriptor | ConvertTo-Json -Depth 10) + [Environment]::NewLine)

$gameTarget = @'
using UnrealBuildTool;

public class RuntimeHostTarget : TargetRules
{
	public RuntimeHostTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("RuntimeHost");
	}
}
'@
Write-Utf8File -Path (Join-Path $hostRoot 'Source\RuntimeHost.Target.cs') -Content ($gameTarget + [Environment]::NewLine)

$editorTarget = @'
using UnrealBuildTool;

public class RuntimeHostEditorTarget : TargetRules
{
	public RuntimeHostEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("RuntimeHost");
	}
}
'@
Write-Utf8File -Path (Join-Path $hostRoot 'Source\RuntimeHostEditor.Target.cs') -Content ($editorTarget + [Environment]::NewLine)

$moduleRules = @'
using UnrealBuildTool;

public class RuntimeHost : ModuleRules
{
	public RuntimeHost(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealBMFont"
			}
		);
	}
}
'@
Write-Utf8File -Path (Join-Path $hostSourceDirectory 'RuntimeHost.Build.cs') -Content ($moduleRules + [Environment]::NewLine)

$escapedVersionName = ([string]$descriptor.VersionName).Replace('\', '\\').Replace('"', '\"')
$moduleSource = @"
#include "BMFontRendering.h"
#include "BMFontText.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

class FRuntimeHostModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		if (!IsRunningCommandlet())
		{
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FRuntimeHostModule::RunReleaseSmoke));
		}
	}

	virtual void ShutdownModule() override
	{
		if (TickerHandle.IsValid())
		{
			FTSTicker::RemoveTicker(TickerHandle);
			TickerHandle.Reset();
		}
		FDefaultGameModuleImpl::ShutdownModule();
	}

private:
	bool RunReleaseSmoke(float)
	{
		UBMFontText* SmokeWidget = NewObject<UBMFontText>(GetTransientPackage());
		const FText ExpectedText = FText::FromString(TEXT("Unreal BMFont $escapedVersionName runtime smoke"));
		SmokeWidget->SetText(ExpectedText);
		SmokeWidget->TakeWidget();

		UMaterialInterface* PackedMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			BMFontRendering::GetDefaultPackedMaterialPath());
		const bool bSmokeSucceeded = IsValid(SmokeWidget)
			&& SmokeWidget->GetText().EqualTo(ExpectedText)
			&& IsValid(PackedMaterial);

		FString MarkerPath;
		if (!FParse::Value(FCommandLine::Get(), TEXT("BMFontSmokeMarker="), MarkerPath)
			|| MarkerPath.IsEmpty())
		{
			MarkerPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealBMFontReleaseSmoke.txt"));
		}
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(MarkerPath), true);
		const FString Marker = FString::Printf(
			TEXT("UNREALBMFONT_RELEASE_SMOKE=%s\nWIDGET_CLASS=%s\nTEXT=%s\nPACKED_MATERIAL=%s\n"),
			bSmokeSucceeded ? TEXT("PASS") : TEXT("FAIL"),
			*SmokeWidget->GetClass()->GetPathName(),
			*SmokeWidget->GetText().ToString(),
			PackedMaterial != nullptr ? *PackedMaterial->GetPathName() : TEXT("NONE"));
		const bool bMarkerWritten = FFileHelper::SaveStringToFile(
			Marker,
			*MarkerPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		UE_LOG(
			LogTemp,
			Display,
			TEXT("UNREALBMFONT_RELEASE_SMOKE: %s (%s)"),
			bSmokeSucceeded && bMarkerWritten ? TEXT("PASS") : TEXT("FAIL"),
			*MarkerPath);
		FPlatformMisc::RequestExit(false);
		return false;
	}

	FTSTicker::FDelegateHandle TickerHandle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FRuntimeHostModule, RuntimeHost, "RuntimeHost");
"@
Write-Utf8File -Path (Join-Path $hostSourceDirectory 'RuntimeHost.cpp') -Content ($moduleSource + [Environment]::NewLine)

$defaultEngine = @'
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Entry
ServerDefaultMap=/Engine/Maps/Entry
'@
Write-Utf8File -Path (Join-Path $hostConfigDirectory 'DefaultEngine.ini') -Content ($defaultEngine + [Environment]::NewLine)

$defaultGame = @'
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=9CB61A8E4D754C43A22595949EDBA38C
ProjectName=Unreal BMFont Runtime Smoke

[/Script/UnrealEd.ProjectPackagingSettings]
UsePakFile=True
bUseIoStore=True
bUseZenStore=False
+DirectoriesToAlwaysCook=(Path="/UnrealBMFont")
'@
Write-Utf8File -Path (Join-Path $hostConfigDirectory 'DefaultGame.ini') -Content ($defaultGame + [Environment]::NewLine)

$results = @()
foreach ($configuration in $Configurations) {
	$archiveDirectory = Join-Path $fullWorkDirectory "Archive-$configuration"
	$uatLog = Join-Path $evidenceDirectory "BuildCookRun-$configuration.log"
	$uatArguments = @(
		'BuildCookRun',
		"-project=$projectFile",
		'-noP4',
		'-utf8output',
		'-unattended',
		'-WaitForUATMutex',
		'-build',
		'-cook',
		'-AdditionalCookerOptions=-SkipZenStore',
		'-stage',
		'-package',
		'-pak',
		'-iostore',
		'-archive',
		"-archivedirectory=$archiveDirectory",
		"-platform=$TargetPlatform",
		"-clientconfig=$configuration"
	)

	Write-Host "Building, cooking, and packaging RuntimeHost ($configuration)..."
	& $uat @uatArguments 2>&1 | Tee-Object -FilePath $uatLog
	$uatExitCode = $LASTEXITCODE
	if ($uatExitCode -ne 0) {
		throw "BuildCookRun failed for $configuration with exit code $uatExitCode. See $uatLog"
	}

	$executables = @(Get-ChildItem -LiteralPath $archiveDirectory -Recurse -Filter 'RuntimeHost.exe' -File)
	$runtimeExecutable = $executables |
		Where-Object { $_.FullName -match '\\RuntimeHost\\Binaries\\Win64\\RuntimeHost\.exe$' } |
		Select-Object -First 1
	if ($null -eq $runtimeExecutable) {
		$runtimeExecutable = $executables | Select-Object -First 1
	}
	if ($null -eq $runtimeExecutable) {
		throw "No packaged RuntimeHost executable was found for $configuration under $archiveDirectory"
	}

	$runtimeLog = Join-Path $evidenceDirectory "Runtime-$configuration.log"
	$markerFile = Join-Path $evidenceDirectory "Runtime-$configuration.txt"
	$runtimeArguments = @(
		'-NullRHI',
		'-Unattended',
		'-NoSound',
		'-NoSplash',
		'-stdout',
		'-FullStdOutLogOutput',
		"-AbsLog=`"$runtimeLog`"",
		"-BMFontSmokeMarker=`"$markerFile`""
	)
	Write-Host "Launching packaged RuntimeHost ($configuration): $($runtimeExecutable.FullName)"
	$runtimeProcess = Start-Process `
		-FilePath $runtimeExecutable.FullName `
		-ArgumentList $runtimeArguments `
		-PassThru `
		-WindowStyle Hidden
	if (-not $runtimeProcess.WaitForExit($LaunchTimeoutSeconds * 1000)) {
		$runtimeProcess.Kill($true)
		throw "Packaged RuntimeHost timed out after $LaunchTimeoutSeconds second(s): $configuration"
	}
	$runtimeProcess.WaitForExit()
	if ($runtimeProcess.ExitCode -ne 0) {
		throw "Packaged RuntimeHost exited with code $($runtimeProcess.ExitCode): $configuration. See $runtimeLog"
	}
	if (-not (Test-Path -LiteralPath $markerFile -PathType Leaf)) {
		throw "Packaged RuntimeHost did not write its deterministic marker: $markerFile"
	}
	$marker = Get-Content -LiteralPath $markerFile -Raw
	if ($marker -notmatch '(?m)^UNREALBMFONT_RELEASE_SMOKE=PASS$' `
		-or $marker -notmatch '(?m)^WIDGET_CLASS=/Script/UnrealBMFont\.BMFontText$' `
		-or $marker -notmatch '(?m)^PACKED_MATERIAL=/UnrealBMFont/M_BMFontPacked\.M_BMFontPacked$') {
		throw "Packaged RuntimeHost marker did not prove widget and packed-material availability: $markerFile"
	}
	$runtimeLogEvidence = if (Test-Path -LiteralPath $runtimeLog -PathType Leaf) {
		$runtimeLog
	} else {
		$null
	}

	$results += [ordered]@{
		Configuration = $configuration
		Executable = $runtimeExecutable.FullName
		BuildCookRunLog = $uatLog
		RuntimeLog = $runtimeLogEvidence
		Marker = $markerFile
	}
	Write-Host "Packaged runtime smoke passed: $configuration"
}

$summaryFile = Join-Path $evidenceDirectory 'summary.json'
$summary = [ordered]@{
	PluginVersion = [string]$descriptor.VersionName
	EngineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
	TargetPlatform = $TargetPlatform
	Configurations = $results
}
Write-Utf8File -Path $summaryFile -Content (($summary | ConvertTo-Json -Depth 10) + [Environment]::NewLine)

Write-Host "Packaged runtime validation passed for $($Configurations -join ', ')."
Write-Host "Evidence: $evidenceDirectory"
Write-Host "Summary: $summaryFile"
