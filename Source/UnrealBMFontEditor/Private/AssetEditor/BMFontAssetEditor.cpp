// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "AssetEditor/BMFontAssetEditor.h"

#include "AssetDefinition.h"
#include "AssetEditor/SBMFontAtlasPreview.h"
#include "BMFontAsset.h"
#include "EditorFramework/AssetImportData.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontAssetEditor"

namespace
{
	const FName SummaryTabId(TEXT("BMFontEditor_Summary"));
	const FName AtlasTabId(TEXT("BMFontEditor_Atlas"));
	const FName GlyphTableTabId(TEXT("BMFontEditor_GlyphTable"));

	FText FormatChannelContent(const EBMFontChannelContent Content)
	{
		switch (Content)
		{
		case EBMFontChannelContent::Glyph: return LOCTEXT("ChannelGlyph", "Glyph");
		case EBMFontChannelContent::Outline: return LOCTEXT("ChannelOutline", "Outline");
		case EBMFontChannelContent::GlyphAndOutline: return LOCTEXT("ChannelGlyphAndOutline", "Glyph + Outline");
		case EBMFontChannelContent::Zero: return LOCTEXT("ChannelZero", "Zero");
		case EBMFontChannelContent::One: return LOCTEXT("ChannelOne", "One");
		default: return LOCTEXT("ChannelUnknown", "Unknown");
		}
	}

	FText FormatDescriptorFormat(const EBMFontDescriptorFormat Format)
	{
		switch (Format)
		{
		case EBMFontDescriptorFormat::Text: return LOCTEXT("FormatText", "Text");
		case EBMFontDescriptorFormat::Xml: return LOCTEXT("FormatXml", "XML");
		case EBMFontDescriptorFormat::Binary: return LOCTEXT("FormatBinary", "Binary v3");
		default: return LOCTEXT("FormatUnknown", "Unknown");
		}
	}
}

void FBMFontAssetEditor::Open(UBMFontAsset* InAsset, const FAssetOpenArgs& OpenArgs)
{
	if (InAsset == nullptr)
	{
		return;
	}

	TSharedRef<FBMFontAssetEditor> Editor = MakeShared<FBMFontAssetEditor>();
	Editor->Init(InAsset, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
}

void FBMFontAssetEditor::Init(
	UBMFontAsset* InAsset,
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	EditingAsset = InAsset;

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_BMFontAssetEditor_Layout_v1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.28f)
				->AddTab(SummaryTabId, ETabState::OpenedTab)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.44f)
				->AddTab(AtlasTabId, ETabState::OpenedTab)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.28f)
				->AddTab(GlyphTableTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		TEXT("BMFontAssetEditorApp"),
		Layout,
		false,
		false,
		InAsset
	);
}

void FBMFontAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "BMFont"));
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(SummaryTabId, FOnSpawnTab::CreateSP(this, &FBMFontAssetEditor::SpawnSummaryTab))
		.SetDisplayName(LOCTEXT("SummaryTab", "Summary"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(AtlasTabId, FOnSpawnTab::CreateSP(this, &FBMFontAssetEditor::SpawnAtlasTab))
		.SetDisplayName(LOCTEXT("AtlasTab", "Atlas"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(GlyphTableTabId, FOnSpawnTab::CreateSP(this, &FBMFontAssetEditor::SpawnGlyphTableTab))
		.SetDisplayName(LOCTEXT("GlyphTableTab", "Glyphs"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
}

void FBMFontAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(SummaryTabId);
	InTabManager->UnregisterTabSpawner(AtlasTabId);
	InTabManager->UnregisterTabSpawner(GlyphTableTabId);
}

FName FBMFontAssetEditor::GetToolkitFName() const
{
	return FName(TEXT("BMFontAssetEditor"));
}

FText FBMFontAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "BMFont");
}

FString FBMFontAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "BMFont ").ToString();
}

FLinearColor FBMFontAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(FColor(68, 128, 230));
}

TSharedRef<SDockTab> FBMFontAssetEditor::SpawnSummaryTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			BuildSummaryPanel()
		];
}

TSharedRef<SDockTab> FBMFontAssetEditor::SpawnAtlasTab(const FSpawnTabArgs& Args)
{
	TSharedPtr<TArray<TSharedPtr<int32>>> PageOptions = MakeShared<TArray<TSharedPtr<int32>>>();
	if (EditingAsset != nullptr)
	{
		for (const FBMFontPage& Page : EditingAsset->FontData.Pages)
		{
			PageOptions->Add(MakeShared<int32>(Page.Id));
		}
	}
	if (PageOptions->Num() > 0)
	{
		CurrentPageId = *(*PageOptions)[0];
	}

	TSharedPtr<STextBlock> PageLabel;
	TSharedRef<SComboBox<TSharedPtr<int32>>> PageSelector = SNew(SComboBox<TSharedPtr<int32>>)
		.OptionsSource(PageOptions.Get())
		.InitiallySelectedItem(PageOptions->Num() > 0 ? (*PageOptions)[0] : TSharedPtr<int32>())
		.OnGenerateWidget_Lambda([](const TSharedPtr<int32>& Item)
		{
			return SNew(STextBlock).Text(
				FText::Format(LOCTEXT("PageOption", "Page {0}"), Item.IsValid() ? *Item : 0)
			);
		})
		.OnSelectionChanged_Lambda([this, PageLabel](const TSharedPtr<int32>& Item, ESelectInfo::Type)
		{
			if (Item.IsValid())
			{
				CurrentPageId = *Item;
				if (AtlasPreview.IsValid())
				{
					AtlasPreview->SetPageId(CurrentPageId);
				}
				if (PageLabel.IsValid())
				{
					PageLabel->SetText(FText::Format(LOCTEXT("PageLabelFmt", "Page {0}"), CurrentPageId));
				}
			}
		})
		[
			SAssignNew(PageLabel, STextBlock)
			.Text(FText::Format(LOCTEXT("PageLabelInitial", "Page {0}"), CurrentPageId))
		];

	TSharedRef<SBMFontAtlasPreview> Preview = SAssignNew(AtlasPreview, SBMFontAtlasPreview)
		.FontAsset(EditingAsset)
		.PageId(CurrentPageId)
		.OnGlyphSelected(FOnBMFontGlyphSelected::CreateSP(this, &FBMFontAssetEditor::HandleGlyphSelected));

	const bool bHasMultiplePages = EditingAsset != nullptr && EditingAsset->FontData.Pages.Num() > 1;

	return SNew(SDockTab)
		.TabRole(ETabRole::MajorTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				bHasMultiplePages
					? static_cast<TSharedRef<SWidget>>(PageSelector)
					: static_cast<TSharedRef<SWidget>>(SNew(SBox))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				Preview
			]
		];
}

TSharedRef<SDockTab> FBMFontAssetEditor::SpawnGlyphTableTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			BuildGlyphTable()
		];
}

TSharedRef<SWidget> FBMFontAssetEditor::BuildSummaryPanel()
{
	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);

	if (EditingAsset == nullptr)
	{
		return SNew(SScrollBox) + SScrollBox::Slot()[Panel];
	}

	const FBMFontData& Data = EditingAsset->FontData;

	AddSummaryRow(Panel, LOCTEXT("SummaryFace", "Face"), FText::FromString(Data.Info.Face));	AddSummaryRow(Panel, LOCTEXT("SummarySize", "Size"), FText::AsNumber(Data.Info.Size));
	AddSummaryRow(Panel, LOCTEXT("SummaryFormat", "Format"), FormatDescriptorFormat(Data.DescriptorFormat));
	AddSummaryRow(Panel, LOCTEXT("SummaryLineHeight", "Line Height"), FText::AsNumber(Data.Common.LineHeight));
	AddSummaryRow(Panel, LOCTEXT("SummaryBase", "Base"), FText::AsNumber(Data.Common.Base));
	AddSummaryRow(
		Panel,
		LOCTEXT("SummaryAtlasSize", "Atlas Size"),
		FText::Format(LOCTEXT("AtlasSizeFmt", "{0} x {1}"), FText::AsNumber(Data.Common.ScaleWidth), FText::AsNumber(Data.Common.ScaleHeight))
	);
	AddSummaryRow(Panel, LOCTEXT("SummaryPages", "Pages"), FText::AsNumber(Data.Pages.Num()));
	AddSummaryRow(Panel, LOCTEXT("SummaryGlyphs", "Glyphs"), FText::AsNumber(Data.Glyphs.Num()));
	AddSummaryRow(Panel, LOCTEXT("SummaryKerning", "Kerning Pairs"), FText::AsNumber(Data.KerningPairs.Num()));
	AddSummaryRow(
		Panel,
		LOCTEXT("SummaryPacked", "Packed"),
		Data.Common.bPacked ? LOCTEXT("PackedYes", "Yes") : LOCTEXT("PackedNo", "No")
	);
	if (Data.Common.bPacked)
	{
		AddSummaryRow(Panel, LOCTEXT("SummaryChannelA", "Alpha Channel"), FormatChannelContent(Data.Common.AlphaChannel));
		AddSummaryRow(Panel, LOCTEXT("SummaryChannelR", "Red Channel"), FormatChannelContent(Data.Common.RedChannel));
		AddSummaryRow(Panel, LOCTEXT("SummaryChannelG", "Green Channel"), FormatChannelContent(Data.Common.GreenChannel));
		AddSummaryRow(Panel, LOCTEXT("SummaryChannelB", "Blue Channel"), FormatChannelContent(Data.Common.BlueChannel));
	}

#if WITH_EDITORONLY_DATA
	if (EditingAsset->AssetImportData != nullptr)
	{
		AddSummaryRow(
			Panel,
			LOCTEXT("SummarySource", "Source"),
			FText::FromString(EditingAsset->AssetImportData->GetFirstFilename())
		);
	}
#endif

	return SNew(SScrollBox) + SScrollBox::Slot()[Panel];
}

void FBMFontAssetEditor::AddSummaryRow(
	const TSharedRef<SVerticalBox>& Panel,
	const FText& Label,
	const FText& Value) const
{
	Panel->Slot()
	.AutoHeight()
	.Padding(8.0f, 3.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.42f)
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.58f)
		[
			SNew(STextBlock)
			.Text(Value)
			.AutoWrapText(true)
		]
	];
}

TSharedRef<SWidget> FBMFontAssetEditor::BuildGlyphTable()
{
	RefreshGlyphRows();

	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	const auto AddColumn = [&Header](const FName Id, const FText& Label, const float Width)
	{
		Header->AddColumn(
			SHeaderRow::Column(Id)
			.DefaultLabel(Label)
			.FixedWidth(Width)
		);
	};
	AddColumn(TEXT("Codepoint"), LOCTEXT("GlyphColumnCodepoint", "Codepoint"), 110.0f);
	AddColumn(TEXT("Advance"), LOCTEXT("GlyphColumnAdvance", "Advance"), 70.0f);
	AddColumn(TEXT("Size"), LOCTEXT("GlyphColumnSize", "Size"), 90.0f);
	AddColumn(TEXT("Offset"), LOCTEXT("GlyphColumnOffset", "Offset"), 90.0f);
	AddColumn(TEXT("Page"), LOCTEXT("GlyphColumnPage", "Page"), 50.0f);

	const auto MakeCell = [](const FString& CellText, const float Width)
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.VAlign(VAlign_Center)
			.Padding(FMargin(2.0f, 0.0f))
			[
				SNew(STextBlock).Text(FText::FromString(CellText))
			];
	};

	return SAssignNew(GlyphListView, SListView<TSharedPtr<int32>>)
		.ListItemsSource(&GlyphRows)
		.HeaderRow(Header)
		.OnGenerateRow_Lambda([this, MakeCell](const TSharedPtr<int32>& Item, const TSharedRef<STableViewBase>& OwnerTable)
		{
			const UBMFontAsset* Font = EditingAsset;
			const FBMFontGlyph* Glyph = (Font != nullptr && Item.IsValid()) ? Font->FindGlyph(*Item) : nullptr;

			FString CodepointText = TEXT("?");
			FString AdvanceText = TEXT("-");
			FString SizeText = TEXT("-");
			FString OffsetText = TEXT("-");
			FString PageText = TEXT("-");
			if (Glyph != nullptr)
			{
				CodepointText = FString::Printf(TEXT("U+%04X  %s"), Glyph->Codepoint, *FString::Chr(Glyph->Codepoint));
				AdvanceText = FString::FromInt(Glyph->XAdvance);
				SizeText = FString::Printf(TEXT("%d x %d"), Glyph->Width, Glyph->Height);
				OffsetText = FString::Printf(TEXT("%d, %d"), Glyph->XOffset, Glyph->YOffset);
				PageText = FString::FromInt(Glyph->Page);
			}

			return SNew(STableRow<TSharedPtr<int32>>, OwnerTable)
				.Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[MakeCell(CodepointText, 110.0f)]
					+ SHorizontalBox::Slot().AutoWidth()[MakeCell(AdvanceText, 70.0f)]
					+ SHorizontalBox::Slot().AutoWidth()[MakeCell(SizeText, 90.0f)]
					+ SHorizontalBox::Slot().AutoWidth()[MakeCell(OffsetText, 90.0f)]
					+ SHorizontalBox::Slot().AutoWidth()[MakeCell(PageText, 50.0f)]
				];
		});
}

void FBMFontAssetEditor::HandleGlyphSelected(const int32 Codepoint)
{
	if (AtlasPreview.IsValid())
	{
		AtlasPreview->SetSelectedCodepoint(Codepoint);
	}
	if (GlyphListView.IsValid())
	{
		const TSharedPtr<int32>* Row = GlyphRows.FindByPredicate(
			[Codepoint](const TSharedPtr<int32>& Candidate)
			{
				return Candidate.IsValid() && *Candidate == Codepoint;
			}
		);
		if (Row != nullptr)
		{
			GlyphListView->SetSelection(*Row);
			GlyphListView->RequestScrollIntoView(*Row);
		}
	}
}

void FBMFontAssetEditor::RefreshGlyphRows()
{
	GlyphRows.Reset();
	if (EditingAsset == nullptr)
	{
		return;
	}

	GlyphRows.Reserve(EditingAsset->FontData.Glyphs.Num());
	for (const TPair<int32, FBMFontGlyph>& Entry : EditingAsset->FontData.Glyphs)
	{
		GlyphRows.Add(MakeShared<int32>(Entry.Key));
	}
	GlyphRows.Sort([](const TSharedPtr<int32>& A, const TSharedPtr<int32>& B)
	{
		return *A < *B;
	});
}

#undef LOCTEXT_NAMESPACE
