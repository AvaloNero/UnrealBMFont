// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontParser.h"
#include "Misc/AutomationTest.h"

namespace
{
	void AddUInt8(TArray<uint8>& Bytes, const uint8 Value)
	{
		Bytes.Add(Value);
	}

	void AddUInt16(TArray<uint8>& Bytes, const uint16 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xFF));
	}

	void AddInt16(TArray<uint8>& Bytes, const int16 Value)
	{
		AddUInt16(Bytes, static_cast<uint16>(Value));
	}

	void AddUInt32(TArray<uint8>& Bytes, const uint32 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 16) & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 24) & 0xFF));
	}

	void AddUtf8String(TArray<uint8>& Bytes, const ANSICHAR* Value)
	{
		while (*Value != '\0')
		{
			Bytes.Add(static_cast<uint8>(*Value++));
		}
		Bytes.Add(0);
	}

	void AddBlock(TArray<uint8>& File, const uint8 Type, const TArray<uint8>& Block)
	{
		AddUInt8(File, Type);
		AddUInt32(File, Block.Num());
		File.Append(Block);
	}

	TArray<uint8> MakeBinaryDescriptor(
		const bool bAddDuplicateKerning = false,
		const bool bAddSecondGlyph = false)
	{
		TArray<uint8> File = { 'B', 'M', 'F', 3 };

		TArray<uint8> Info;
		AddInt16(Info, 24);
		AddUInt8(Info, 0x03);
		AddUInt8(Info, 0);
		AddUInt16(Info, 100);
		AddUInt8(Info, 1);
		AddUInt8(Info, 1);
		AddUInt8(Info, 2);
		AddUInt8(Info, 3);
		AddUInt8(Info, 4);
		AddUInt8(Info, 1);
		AddUInt8(Info, 1);
		AddUInt8(Info, 0);
		AddUtf8String(Info, "Binary Test");
		AddBlock(File, 1, Info);

		TArray<uint8> Common;
		AddUInt16(Common, 32);
		AddUInt16(Common, 24);
		AddUInt16(Common, 64);
		AddUInt16(Common, 64);
		AddUInt16(Common, 1);
		AddUInt8(Common, 0);
		AddUInt8(Common, 0);
		AddUInt8(Common, 4);
		AddUInt8(Common, 4);
		AddUInt8(Common, 4);
		AddBlock(File, 2, Common);

		TArray<uint8> Pages;
		AddUtf8String(Pages, "atlas.png");
		AddBlock(File, 3, Pages);

		TArray<uint8> Chars;
		AddUInt32(Chars, 65);
		AddUInt16(Chars, 2);
		AddUInt16(Chars, 3);
		AddUInt16(Chars, 8);
		AddUInt16(Chars, 10);
		AddInt16(Chars, -1);
		AddInt16(Chars, 2);
		AddInt16(Chars, 9);
		AddUInt8(Chars, 0);
		AddUInt8(Chars, 15);
		if (bAddSecondGlyph)
		{
			AddUInt32(Chars, 66);
			AddUInt16(Chars, 12);
			AddUInt16(Chars, 3);
			AddUInt16(Chars, 8);
			AddUInt16(Chars, 10);
			AddInt16(Chars, 0);
			AddInt16(Chars, 2);
			AddInt16(Chars, 9);
			AddUInt8(Chars, 0);
			AddUInt8(Chars, 15);
		}
		AddBlock(File, 4, Chars);

		TArray<uint8> Kernings;
		AddUInt32(Kernings, 65);
		AddUInt32(Kernings, 65);
		AddInt16(Kernings, -2);
		if (bAddDuplicateKerning)
		{
			AddUInt32(Kernings, 65);
			AddUInt32(Kernings, 65);
			AddInt16(Kernings, -3);
		}
		AddBlock(File, 5, Kernings);
		return File;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontTextDescriptorTest,
	"UnrealBMFont.Parser.TextDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontTextDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Descriptor =
		TEXT("\xFEFFinfo face=\"Open Font\" size=32 bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 padding=1,2,3,4 spacing=5,6 outline=1\n")
		TEXT("common pages=2 scaleH=64 lineHeight=32 scaleW=128 base=24 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n")
		TEXT("page file=\"atlas zero.png\" id=0\n")
		TEXT("page id=1 file=\"atlas-one.png\"\n")
		TEXT("chars count=3\n")
		TEXT("char xadvance=8 id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 page=0 chnl=15\n")
		TEXT("char id=65 x=1 y=2 width=8 height=10 xoffset=-1 yoffset=3 xadvance=11 page=0 chnl=15\n")
		TEXT("char id=128512 x=16 y=2 width=12 height=12 xoffset=0 yoffset=1 xadvance=13 page=1 chnl=15\n")
		TEXT("kernings count=1\n")
		TEXT("kerning second=65 amount=-2 first=65\n");

	const FBMFontParseResult Result = FBMFontParser::ParseText(Descriptor);
	TestTrue(TEXT("Text descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Text);
	TestEqual(TEXT("Face preserves spaces"), Result.Data.Info.Face, FString(TEXT("Open Font")));
	TestEqual(TEXT("Padding right"), Result.Data.Info.PaddingRight, 2);
	TestEqual(TEXT("Spacing"), Result.Data.Info.Spacing, FIntPoint(5, 6));
	TestEqual(TEXT("Page count"), Result.Data.Pages.Num(), 2);
	TestEqual(TEXT("Glyph count"), Result.Data.Glyphs.Num(), 3);
	TestTrue(TEXT("Supplementary Unicode glyph exists"), Result.Data.Glyphs.Contains(0x1F600));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("ASCII glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Field order is independent"), Glyph->XAdvance, 11);
	}
	TestEqual(TEXT("Kerning count"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("Kerning amount"), Result.Data.KerningPairs[0].Amount, -2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontXmlDescriptorTest,
	"UnrealBMFont.Parser.XmlDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontXmlDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Xml =
		TEXT("<?xml version=\"1.0\"?>")
		TEXT("<font><info face=\"XML Font\" size=\"16\" unicode=\"1\"/>")
		TEXT("<common lineHeight=\"20\" base=\"15\" scaleW=\"64\" scaleH=\"64\" pages=\"1\" packed=\"0\"/>")
		TEXT("<pages><page id=\"0\" file=\"atlas.png\"/></pages>")
		TEXT("<chars count=\"1\"><char id=\"65\" x=\"1\" y=\"2\" width=\"8\" height=\"10\" xoffset=\"0\" yoffset=\"1\" xadvance=\"9\" page=\"0\" chnl=\"15\"/></chars>")
		TEXT("<kernings count=\"0\"/></font>");

	const FBMFontParseResult Result = FBMFontParser::ParseXml(Xml);
	TestTrue(TEXT("XML descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Xml);
	TestEqual(TEXT("Face"), Result.Data.Info.Face, FString(TEXT("XML Font")));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("Glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Glyph advance"), Glyph->XAdvance, 9);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontBinaryDescriptorTest,
	"UnrealBMFont.Parser.BinaryDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontBinaryDescriptorTest::RunTest(const FString& Parameters)
{
	const TArray<uint8> Descriptor = MakeBinaryDescriptor();
	const FBMFontParseResult Result = FBMFontParser::Parse(Descriptor);
	TestTrue(TEXT("Binary descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Binary);
	TestEqual(TEXT("Face"), Result.Data.Info.Face, FString(TEXT("Binary Test")));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("Binary glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Glyph offset"), Glyph->XOffset, -1);
	}
	TestEqual(TEXT("Binary kerning count"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("Kerning"), Result.Data.KerningPairs[0].Amount, -2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontBinaryDuplicateKerningTest,
	"UnrealBMFont.Parser.BinaryDuplicateKerning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontBinaryDuplicateKerningTest::RunTest(const FString& Parameters)
{
	const FBMFontParseResult Result = FBMFontParser::Parse(MakeBinaryDescriptor(true));
	TestTrue(TEXT("Descriptor with duplicate binary kerning parses"), Result.IsSuccess());
	TestEqual(TEXT("Duplicate pair is deduplicated"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("The last binary kerning record wins"), Result.Data.KerningPairs[0].Amount, -3);
	}
	TestTrue(
		TEXT("Duplicate binary kerning emits a warning"),
		Result.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Severity == EBMFontParseMessageSeverity::Warning
					&& Message.Message.Contains(TEXT("Duplicate kerning pair"));
			}
		)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontInvalidDescriptorTest,
	"UnrealBMFont.Parser.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontInvalidDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Descriptor =
		TEXT("common lineHeight=0 base=0 scaleW=0 scaleH=0 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult Result = FBMFontParser::ParseText(Descriptor);
	TestFalse(TEXT("Invalid atlas is rejected"), Result.IsSuccess());
	TestTrue(TEXT("Validation emits errors"), Result.HasErrors());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontAtlasOverflowValidationTest,
	"UnrealBMFont.Parser.AtlasOverflowValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontAtlasOverflowValidationTest::RunTest(const FString& Parameters)
{
	const FString Descriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=2147483646 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");

	const FBMFontParseResult Result = FBMFontParser::ParseText(Descriptor);
	TestFalse(TEXT("Overflowing atlas rectangle is rejected"), Result.IsSuccess());
	TestTrue(
		TEXT("Overflowing atlas rectangle reports the bounds error"),
		Result.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Severity == EBMFontParseMessageSeverity::Error
					&& Message.Message.Contains(TEXT("outside the declared atlas"));
			}
		)
	);

	FBMFontCommon Common;
	Common.ScaleWidth = 64;
	Common.ScaleHeight = 64;
	FBMFontGlyph OverflowingGlyph;
	OverflowingGlyph.X = MAX_int32 - 1;
	OverflowingGlyph.Y = 0;
	OverflowingGlyph.Width = 8;
	OverflowingGlyph.Height = 8;
	const FBox2D SafeUvRegion = OverflowingGlyph.GetUvRegion(Common);
	TestEqual(TEXT("Invalid public UV query returns a zero minimum"), SafeUvRegion.Min, FVector2D::ZeroVector);
	TestEqual(TEXT("Invalid public UV query returns a zero maximum"), SafeUvRegion.Max, FVector2D::ZeroVector);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontParserResourceLimitsTest,
	"UnrealBMFont.Parser.ResourceLimits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontParserResourceLimitsTest::RunTest(const FString& Parameters)
{
	FBMFontParserLimits Limits;
	Limits.MaxGlyphs = 1;
	const FString TextDescriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=2\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n")
		TEXT("char id=66 x=8 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult TextResult = FBMFontParser::ParseText(TextDescriptor, Limits);
	TestFalse(TEXT("Text glyph record limit is enforced"), TextResult.IsSuccess());
	TestTrue(
		TEXT("Text glyph record limit emits an actionable diagnostic"),
		TextResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limit of 1 glyph record"));
			}
		)
	);

	const FBMFontParseResult BinaryResult = FBMFontParser::ParseBinary(
		MakeBinaryDescriptor(false, true),
		Limits
	);
	TestFalse(TEXT("Binary glyph record limit is enforced"), BinaryResult.IsSuccess());
	TestTrue(
		TEXT("Binary glyph record limit emits an actionable diagnostic"),
		BinaryResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("binary chars blocks"));
			}
		)
	);

	FBMFontParserLimits ByteLimits;
	ByteLimits.MaxDescriptorBytes = 3;
	const TArray<uint8> OversizedBytes = { 'a', 'b', 'c', 'd' };
	const FBMFontParseResult ByteResult = FBMFontParser::Parse(OversizedBytes, ByteLimits);
	TestFalse(TEXT("Descriptor byte limit is enforced before format detection"), ByteResult.IsSuccess());
	TestTrue(TEXT("Descriptor byte limit emits an error"), ByteResult.HasErrors());

	FBMFontParserLimits PageLimits;
	PageLimits.MaxPages = 1;
	const FString TwoPageDescriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=2 packed=0\n")
		TEXT("page id=0 file=\"atlas-0.png\"\n")
		TEXT("page id=1 file=\"atlas-1.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult PageResult = FBMFontParser::ParseText(TwoPageDescriptor, PageLimits);
	TestFalse(TEXT("Page record limit is enforced"), PageResult.IsSuccess());
	TestTrue(
		TEXT("Page record limit emits an actionable diagnostic"),
		PageResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limit of 1 page record"));
			}
		)
	);

	const FString SparsePageIdDescriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=4096 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=4096 chnl=15\n");
	const FBMFontParseResult SparsePageIdResult = FBMFontParser::ParseText(SparsePageIdDescriptor);
	TestTrue(TEXT("Page record limits do not restrict non-negative page ID values"), SparsePageIdResult.IsSuccess());

	FBMFontParserLimits KerningLimits;
	KerningLimits.MaxKerningPairs = 1;
	const FString TwoKerningDescriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n")
		TEXT("kernings count=2\n")
		TEXT("kerning first=65 second=65 amount=-1\n")
		TEXT("kerning first=65 second=66 amount=-2\n");
	const FBMFontParseResult KerningResult = FBMFontParser::ParseText(TwoKerningDescriptor, KerningLimits);
	TestFalse(TEXT("Kerning record limit is enforced"), KerningResult.IsSuccess());
	TestTrue(
		TEXT("Kerning record limit emits an actionable diagnostic"),
		KerningResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limit of 1 kerning record"));
			}
		)
	);

	FBMFontParserLimits AtlasLimits;
	AtlasLimits.MaxAtlasDimension = 32;
	const FString AtlasDescriptor =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult AtlasResult = FBMFontParser::ParseText(AtlasDescriptor, AtlasLimits);
	TestFalse(TEXT("Atlas dimension limit is enforced"), AtlasResult.IsSuccess());
	TestTrue(
		TEXT("Atlas dimension limit emits an actionable diagnostic"),
		AtlasResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("32-pixel dimension limit"));
			}
		)
	);

	FBMFontParserLimits TotalAtlasLimits;
	TotalAtlasLimits.MaxTotalAtlasPixels = 4096;
	const FBMFontParseResult TotalAtlasResult = FBMFontParser::ParseText(TwoPageDescriptor, TotalAtlasLimits);
	TestFalse(TEXT("Total multi-page atlas pixel limit is enforced"), TotalAtlasResult.IsSuccess());
	TestTrue(
		TEXT("Total atlas pixel limit emits an actionable diagnostic"),
		TotalAtlasResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("total limit of 4096 pixel"));
			}
		)
	);

	FBMFontParserLimits TextShapeLimits;
	TextShapeLimits.MaxTextLines = 2;
	const FBMFontParseResult TextShapeResult = FBMFontParser::ParseText(AtlasDescriptor, TextShapeLimits);
	TestFalse(TEXT("Text line limit is enforced before tokenization"), TextShapeResult.IsSuccess());
	TestTrue(
		TEXT("Text line limit emits an actionable diagnostic"),
		TextShapeResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("2-line limit"));
			}
		)
	);

	FBMFontParserLimits FileNameLimits;
	FileNameLimits.MaxPageFileCharacters = 4;
	const FBMFontParseResult FileNameResult = FBMFontParser::ParseBinary(MakeBinaryDescriptor(), FileNameLimits);
	TestFalse(TEXT("Binary page file-name limit is enforced during decoding"), FileNameResult.IsSuccess());
	TestTrue(
		TEXT("Binary page file-name limit emits an actionable diagnostic"),
		FileNameResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("page file name"));
			}
		)
	);

	FBMFontParserLimits InvalidLimits;
	InvalidLimits.MaxPages = 0;
	const FBMFontParseResult InvalidLimitsResult = FBMFontParser::ParseText(AtlasDescriptor, InvalidLimits);
	TestFalse(TEXT("Invalid parser limits are rejected"), InvalidLimitsResult.IsSuccess());
	TestTrue(
		TEXT("Invalid parser limits emit a configuration diagnostic"),
		InvalidLimitsResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limits are invalid"));
			}
		)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontXmlDoctypeRejectionTest,
	"UnrealBMFont.Parser.XmlDoctypeRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontXmlDoctypeRejectionTest::RunTest(const FString& Parameters)
{
	const FString Xml = TEXT("<!DOCTYPE font [<!ENTITY x \"atlas.png\">]><font/>");
	const FBMFontParseResult Result = FBMFontParser::ParseXml(Xml);
	TestFalse(TEXT("DOCTYPE input is rejected"), Result.IsSuccess());
	TestTrue(
		TEXT("DOCTYPE rejection is explicit"),
		Result.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("DOCTYPE"));
			}
		)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontXmlStructuralLimitsTest,
	"UnrealBMFont.Parser.XmlStructuralLimits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontXmlStructuralLimitsTest::RunTest(const FString& Parameters)
{
	FBMFontParserLimits ElementLimits;
	ElementLimits.MaxXmlElements = 3;
	const FBMFontParseResult ElementResult = FBMFontParser::ParseXml(
		TEXT("<font><!-- Author's font --><junk/><junk/><junk/></font>"),
		ElementLimits
	);
	TestFalse(TEXT("An apostrophe in a comment cannot bypass the XML element limit"), ElementResult.IsSuccess());
	TestTrue(
		TEXT("XML element limit emits an actionable diagnostic"),
		ElementResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limit of 3 element"));
			}
		)
	);

	FBMFontParserLimits AttributeLimits;
	AttributeLimits.MaxXmlAttributes = 1;
	const FBMFontParseResult AttributeResult = FBMFontParser::ParseXml(
		TEXT("<font first=\"1\" second='2'/>"),
		AttributeLimits
	);
	TestFalse(TEXT("XML attribute limit is enforced before DOM parsing"), AttributeResult.IsSuccess());
	TestTrue(
		TEXT("XML attribute limit emits an actionable diagnostic"),
		AttributeResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("limit of 1 attribute"));
			}
		)
	);

	FBMFontParserLimits IgnoredMarkupLimits;
	IgnoredMarkupLimits.MaxXmlElements = 1;
	IgnoredMarkupLimits.MaxXmlAttributes = 1;
	const FBMFontParseResult IgnoredMarkupResult = FBMFontParser::ParseXml(
		TEXT("<font><!-- fake=\"value\" <junk attr='x'/> --><![CDATA[ fake=\"value\" <junk attr='x'/> ]]></font>"),
		IgnoredMarkupLimits
	);
	TestFalse(
		TEXT("Comments and CDATA do not consume XML structural limits"),
		IgnoredMarkupResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("XML descriptor exceeds the configured limit"));
			}
		)
	);

	for (const FString MalformedXml : {
		FString(TEXT("<font><!-- unterminated")),
		FString(TEXT("<font><![CDATA[unterminated")),
		FString(TEXT("<?xml version='1.0'<font/>")),
		FString(TEXT("<font face='unterminated></font>"))
	})
	{
		const FBMFontParseResult MalformedResult = FBMFontParser::ParseXml(MalformedXml);
		TestFalse(TEXT("Unterminated XML markup is rejected before DOM parsing"), MalformedResult.IsSuccess());
		TestTrue(
			TEXT("Unterminated XML markup emits an actionable diagnostic"),
			MalformedResult.Messages.ContainsByPredicate(
				[](const FBMFontParseMessage& Message)
				{
					return Message.Message.Contains(TEXT("unterminated"));
				}
			)
		);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontMalformedCorpusTest,
	"UnrealBMFont.Parser.MalformedCorpus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontMalformedCorpusTest::RunTest(const FString& Parameters)
{
	const TArray<uint8> ValidBinary = MakeBinaryDescriptor(true, true);
	for (int32 PrefixLength = 0; PrefixLength < ValidBinary.Num(); ++PrefixLength)
	{
		const FBMFontParseResult Result = FBMFontParser::ParseBinary(
			TArrayView<const uint8>(ValidBinary.GetData(), PrefixLength)
		);
		TestTrue(
			FString::Printf(TEXT("Truncated binary prefix %d keeps diagnostics bounded"), PrefixLength),
			Result.Messages.Num() <= 256
		);
	}

	uint32 State = 0xC001D00D;
	for (int32 CaseIndex = 0; CaseIndex < 128; ++CaseIndex)
	{
		State = State * 1664525u + 1013904223u;
		const int32 ByteCount = static_cast<int32>(State % 513u);
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(ByteCount);
		for (uint8& Byte : Bytes)
		{
			State = State * 1664525u + 1013904223u;
			Byte = static_cast<uint8>(State >> 24);
		}

		const FBMFontParseResult Result = FBMFontParser::Parse(Bytes);
		TestTrue(
			FString::Printf(TEXT("Fixed-seed malformed case %d keeps diagnostics bounded"), CaseIndex),
			Result.Messages.Num() <= 256
		);
	}

	const TArray<uint8> OversizedBlock = {
		'B', 'M', 'F', 3,
		4, 0xFF, 0xFF, 0xFF, 0x7F
	};
	const FBMFontParseResult OversizedBlockResult = FBMFontParser::ParseBinary(OversizedBlock);
	TestFalse(TEXT("A block with an impossible declared size is rejected"), OversizedBlockResult.IsSuccess());
	TestTrue(
		TEXT("Impossible binary block size reports an end-of-file error"),
		OversizedBlockResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("extends past the end"));
			}
		)
	);

	FString WarningFlood =
		TEXT("common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	for (int32 Index = 0; Index < 400; ++Index)
	{
		WarningFlood += FString::Printf(TEXT("unknown%d value=1\n"), Index);
	}
	const FBMFontParseResult WarningFloodResult = FBMFontParser::ParseText(WarningFlood);
	TestTrue(TEXT("Warning-only diagnostic flooding remains successful"), WarningFloodResult.IsSuccess());
	TestEqual(TEXT("Diagnostic flooding is capped"), WarningFloodResult.Messages.Num(), 256);
	TestTrue(
		TEXT("Diagnostic cap reports warning suppression without inventing an error"),
		WarningFloodResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("additional messages were suppressed"))
					&& Message.Severity == EBMFontParseMessageSeverity::Warning;
			}
		)
	);

	const FString ErrorAfterWarningFlood = WarningFlood
		+ TEXT("char id=66 x=invalid y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult SuppressedErrorResult = FBMFontParser::ParseText(ErrorAfterWarningFlood);
	TestTrue(TEXT("An error after the warning cap is still retained"), SuppressedErrorResult.HasErrors());
	TestTrue(
		TEXT("The suppression entry is promoted when a suppressed diagnostic is an error"),
		SuppressedErrorResult.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Message.Contains(TEXT("additional messages were suppressed"))
					&& Message.Severity == EBMFontParseMessageSeverity::Error;
			}
		)
	);
	return true;
}

#endif
