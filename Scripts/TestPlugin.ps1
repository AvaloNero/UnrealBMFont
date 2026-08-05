# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$EngineRoot,

	[Parameter(Mandatory = $true)]
	[string]$Project,

	[string]$ReportDirectory,

	[string]$LogFile,

	[string]$TestFilter = 'UnrealBMFont'
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
$editor = Join-Path $resolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
	throw "UnrealEditor-Cmd.exe was not found under EngineRoot: $resolvedEngineRoot"
}

$resolvedProject = (Resolve-Path -LiteralPath $Project).Path
if (-not $resolvedProject.EndsWith('.uproject', [System.StringComparison]::OrdinalIgnoreCase)) {
	throw "Project must point to a .uproject file: $resolvedProject"
}

$pluginRoot = Split-Path -Parent $PSScriptRoot
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if ([string]::IsNullOrWhiteSpace($ReportDirectory)) {
	$ReportDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "UnrealBMFont\AutomationReports\AutomationReports-$stamp"
}
$fullReportDirectory = [System.IO.Path]::GetFullPath($ReportDirectory)
if (Test-IsPathInsideDirectory -Candidate $fullReportDirectory -Parent $pluginRoot) {
	throw "ReportDirectory must be outside the plugin source tree so reports cannot be staged as plugin inputs: $fullReportDirectory"
}
if (Test-Path -LiteralPath $fullReportDirectory) {
	$existingReports = @(Get-ChildItem -LiteralPath $fullReportDirectory -Force)
	if ($existingReports.Count -gt 0) {
		throw "ReportDirectory must be empty to prevent stale results: $fullReportDirectory"
	}
} else {
	New-Item -ItemType Directory -Path $fullReportDirectory | Out-Null
}

if ([string]::IsNullOrWhiteSpace($LogFile)) {
	$LogFile = Join-Path $fullReportDirectory 'UnrealBMFontTests.log'
}
$fullLogFile = [System.IO.Path]::GetFullPath($LogFile)
if (Test-IsPathInsideDirectory -Candidate $fullLogFile -Parent $pluginRoot) {
	throw "LogFile must be outside the plugin source tree so logs cannot be staged as plugin inputs: $fullLogFile"
}
$logDirectory = Split-Path -Parent $fullLogFile
if (-not (Test-Path -LiteralPath $logDirectory)) {
	New-Item -ItemType Directory -Path $logDirectory | Out-Null
}

$arguments = @(
	$resolvedProject,
	'-Unattended',
	'-NoSplash',
	'-NullRHI',
	'-NoSound',
	'-NoP4',
	'-NoPause',
	'-stdout',
	'-FullStdOutLogOutput',
	"-ExecCmds=Automation RunTests $TestFilter",
	'-TestExit=Automation Test Queue Empty',
	"-ReportExportPath=$fullReportDirectory",
	"-AbsLog=$fullLogFile"
)

& $editor @arguments
$editorExitCode = $LASTEXITCODE
if ($editorExitCode -ne 0) {
	throw "UnrealEditor-Cmd exited with code $editorExitCode. See $fullLogFile"
}

$reportFile = Join-Path $fullReportDirectory 'index.json'
if (-not (Test-Path -LiteralPath $reportFile -PathType Leaf)) {
	throw "Automation did not create index.json. See $fullLogFile"
}

$report = Get-Content -LiteralPath $reportFile -Raw | ConvertFrom-Json
$executedCount = [int]$report.succeeded + [int]$report.succeededWithWarnings + [int]$report.failed
if ($executedCount -eq 0) {
	throw "No tests matched '$TestFilter'. See $reportFile"
}
if ([int]$report.failed -gt 0 -or [int]$report.notRun -gt 0) {
	$failedNames = @($report.tests | Where-Object { $_.state -ne 'Success' } | ForEach-Object { $_.fullTestPath })
	throw "Automation failed: $($failedNames -join ', '). See $reportFile"
}

Write-Host "Automation passed: $executedCount test(s), $($report.succeededWithWarnings) warning result(s)."
Write-Host "Report: $reportFile"
