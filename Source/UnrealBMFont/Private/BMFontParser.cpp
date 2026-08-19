// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontParser.h"

#include "Containers/StringConv.h"
#include "Misc/FileHelper.h"
#include "XmlFile.h"
#include "XmlNode.h"

namespace
{
	using FBMFontAttributes = TMap<FString, FString>;

	void AddMessage(
		FBMFontParseResult& Result,
		const EBMFontParseMessageSeverity Severity,
		const FString& Message,
		const int32 Line = INDEX_NONE)
	{
		FBMFontParseMessage& Entry = Result.Messages.AddDefaulted_GetRef();
		Entry.Severity = Severity;
		Entry.Line = Line;
		Entry.Message = Message;
	}

	void AddError(FBMFontParseResult& Result, const FString& Message, const int32 Line = INDEX_NONE)
	{
		AddMessage(Result, EBMFontParseMessageSeverity::Error, Message, Line);
	}

	void AddWarning(FBMFontParseResult& Result, const FString& Message, const int32 Line = INDEX_NONE)
	{
		AddMessage(Result, EBMFontParseMessageSeverity::Warning, Message, Line);
	}

	bool ReadString(
		const FBMFontAttributes& Attributes,
		const TCHAR* Key,
		FString& OutValue,
		FBMFontParseResult& Result,
		const FString& Context,
		const int32 Line,
		const bool bRequired)
	{
		if (const FString* Value = Attributes.Find(Key))
		{
			OutValue = *Value;
			return true;
		}

		if (bRequired)
		{
			AddError(Result, FString::Printf(TEXT("%s is missing required field '%s'."), *Context, Key), Line);
		}
		return false;
	}

	bool ReadInt(
		const FBMFontAttributes& Attributes,
		const TCHAR* Key,
		int32& OutValue,
		FBMFontParseResult& Result,
		const FString& Context,
		const int32 Line,
		const bool bRequired)
	{
		const FString* Value = Attributes.Find(Key);
		if (Value == nullptr)
		{
			if (bRequired)
			{
				AddError(Result, FString::Printf(TEXT("%s is missing required field '%s'."), *Context, Key), Line);
			}
			return false;
		}

		if (!LexTryParseString(OutValue, **Value))
		{
			AddError(
				Result,
				FString::Printf(TEXT("%s field '%s' is not a valid integer: '%s'."), *Context, Key, **Value),
				Line
			);
			return false;
		}

		return true;
	}

	void ReadBool(
		const FBMFontAttributes& Attributes,
		const TCHAR* Key,
		bool& OutValue,
		FBMFontParseResult& Result,
		const FString& Context,
		const int32 Line)
	{
		int32 IntegerValue = OutValue ? 1 : 0;
		if (ReadInt(Attributes, Key, IntegerValue, Result, Context, Line, false))
		{
			OutValue = IntegerValue != 0;
		}
	}

	bool ReadIntList(
		const FBMFontAttributes& Attributes,
		const TCHAR* Key,
		const int32 ExpectedCount,
		TArray<int32>& OutValues,
		FBMFontParseResult& Result,
		const FString& Context,
		const int32 Line)
	{
		const FString* Value = Attributes.Find(Key);
		if (Value == nullptr)
		{
			return false;
		}

		TArray<FString> Parts;
		Value->ParseIntoArray(Parts, TEXT(","), false);
		if (Parts.Num() != ExpectedCount)
		{
			AddError(
				Result,
				FString::Printf(TEXT("%s field '%s' must contain %d comma-separated integers."), *Context, Key, ExpectedCount),
				Line
			);
			return false;
		}

		OutValues.Reset(ExpectedCount);
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
			int32 ParsedValue = 0;
			if (!LexTryParseString(ParsedValue, *Part))
			{
				AddError(
					Result,
					FString::Printf(TEXT("%s field '%s' contains an invalid integer: '%s'."), *Context, Key, *Part),
					Line
				);
				return false;
			}
			OutValues.Add(ParsedValue);
		}

		return true;
	}

	EBMFontChannelContent ToChannelContent(const int32 Value)
	{
		switch (Value)
		{
		case 0:
			return EBMFontChannelContent::Glyph;
		case 1:
			return EBMFontChannelContent::Outline;
		case 2:
			return EBMFontChannelContent::GlyphAndOutline;
		case 3:
			return EBMFontChannelContent::Zero;
		case 4:
			return EBMFontChannelContent::One;
		default:
			return EBMFontChannelContent::Unknown;
		}
	}

	void ParseInfoAttributes(
		const FBMFontAttributes& Attributes,
		FBMFontParseResult& Result,
		const int32 Line)
	{
		FBMFontInfo& Info = Result.Data.Info;
		const FString Context(TEXT("info record"));
		ReadString(Attributes, TEXT("face"), Info.Face, Result, Context, Line, false);
		ReadInt(Attributes, TEXT("size"), Info.Size, Result, Context, Line, false);
		ReadBool(Attributes, TEXT("bold"), Info.bBold, Result, Context, Line);
		ReadBool(Attributes, TEXT("italic"), Info.bItalic, Result, Context, Line);
		ReadBool(Attributes, TEXT("unicode"), Info.bUnicode, Result, Context, Line);
		ReadBool(Attributes, TEXT("smooth"), Info.bSmooth, Result, Context, Line);
		ReadString(Attributes, TEXT("charset"), Info.CharacterSet, Result, Context, Line, false);
		ReadInt(Attributes, TEXT("stretchH"), Info.StretchHeight, Result, Context, Line, false);
		ReadInt(Attributes, TEXT("aa"), Info.Supersampling, Result, Context, Line, false);
		ReadInt(Attributes, TEXT("outline"), Info.Outline, Result, Context, Line, false);

		TArray<int32> Values;
		if (ReadIntList(Attributes, TEXT("padding"), 4, Values, Result, Context, Line))
		{
			Info.PaddingUp = Values[0];
			Info.PaddingRight = Values[1];
			Info.PaddingDown = Values[2];
			Info.PaddingLeft = Values[3];
		}

		if (ReadIntList(Attributes, TEXT("spacing"), 2, Values, Result, Context, Line))
		{
			Info.Spacing = FIntPoint(Values[0], Values[1]);
		}
	}

	void ParseCommonAttributes(
		const FBMFontAttributes& Attributes,
		FBMFontParseResult& Result,
		const int32 Line)
	{
		FBMFontCommon& Common = Result.Data.Common;
		const FString Context(TEXT("common record"));
		ReadInt(Attributes, TEXT("lineHeight"), Common.LineHeight, Result, Context, Line, true);
		ReadInt(Attributes, TEXT("base"), Common.Base, Result, Context, Line, true);
		ReadInt(Attributes, TEXT("scaleW"), Common.ScaleWidth, Result, Context, Line, true);
		ReadInt(Attributes, TEXT("scaleH"), Common.ScaleHeight, Result, Context, Line, true);
		ReadInt(Attributes, TEXT("pages"), Common.PageCount, Result, Context, Line, true);
		ReadBool(Attributes, TEXT("packed"), Common.bPacked, Result, Context, Line);

		int32 Channel = 0;
		if (ReadInt(Attributes, TEXT("alphaChnl"), Channel, Result, Context, Line, false))
		{
			Common.AlphaChannel = ToChannelContent(Channel);
		}
		if (ReadInt(Attributes, TEXT("redChnl"), Channel, Result, Context, Line, false))
		{
			Common.RedChannel = ToChannelContent(Channel);
		}
		if (ReadInt(Attributes, TEXT("greenChnl"), Channel, Result, Context, Line, false))
		{
			Common.GreenChannel = ToChannelContent(Channel);
		}
		if (ReadInt(Attributes, TEXT("blueChnl"), Channel, Result, Context, Line, false))
		{
			Common.BlueChannel = ToChannelContent(Channel);
		}
	}

	void ParsePageAttributes(
		const FBMFontAttributes& Attributes,
		FBMFontParseResult& Result,
		const int32 Line)
	{
		FBMFontPage Page;
		const FString Context(TEXT("page record"));
		const bool bHasId = ReadInt(Attributes, TEXT("id"), Page.Id, Result, Context, Line, true);
		const bool bHasFile = ReadString(Attributes, TEXT("file"), Page.File, Result, Context, Line, true);
		if (!bHasId || !bHasFile)
		{
			return;
		}

		if (Result.Data.Pages.ContainsByPredicate([&Page](const FBMFontPage& Existing) { return Existing.Id == Page.Id; }))
		{
			AddError(Result, FString::Printf(TEXT("Duplicate page id %d."), Page.Id), Line);
			return;
		}

		Result.Data.Pages.Add(MoveTemp(Page));
	}

	void ParseGlyphAttributes(
		const FBMFontAttributes& Attributes,
		FBMFontParseResult& Result,
		const int32 Line)
	{
		FBMFontGlyph Glyph;
		const FString Context(TEXT("char record"));
		bool bValid = true;
		bValid &= ReadInt(Attributes, TEXT("id"), Glyph.Codepoint, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("x"), Glyph.X, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("y"), Glyph.Y, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("width"), Glyph.Width, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("height"), Glyph.Height, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("xoffset"), Glyph.XOffset, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("yoffset"), Glyph.YOffset, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("xadvance"), Glyph.XAdvance, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("page"), Glyph.Page, Result, Context, Line, true);
		ReadInt(Attributes, TEXT("chnl"), Glyph.Channel, Result, Context, Line, false);
		if (!bValid)
		{
			return;
		}

		if (Result.Data.Glyphs.Contains(Glyph.Codepoint))
		{
			AddWarning(Result, FString::Printf(TEXT("Duplicate glyph id %d; the last record wins."), Glyph.Codepoint), Line);
		}
		Result.Data.Glyphs.Add(Glyph.Codepoint, Glyph);
	}

	void AddKerningPair(
		const FBMFontKerningPair& Pair,
		FBMFontParseResult& Result,
		TMap<uint64, int32>& KerningIndices,
		const int32 Line = INDEX_NONE)
	{
		const uint64 Key = FBMFontKerningPair::MakeKey(Pair.First, Pair.Second);
		if (const int32* ExistingIndex = KerningIndices.Find(Key))
		{
			AddWarning(
				Result,
				FString::Printf(TEXT("Duplicate kerning pair %d/%d; the last record wins."), Pair.First, Pair.Second),
				Line
			);
			Result.Data.KerningPairs[*ExistingIndex] = Pair;
			return;
		}

		KerningIndices.Add(Key, Result.Data.KerningPairs.Num());
		Result.Data.KerningPairs.Add(Pair);
	}

	void ParseKerningAttributes(
		const FBMFontAttributes& Attributes,
		FBMFontParseResult& Result,
		TMap<uint64, int32>& KerningIndices,
		const int32 Line)
	{
		FBMFontKerningPair Pair;
		const FString Context(TEXT("kerning record"));
		bool bValid = true;
		bValid &= ReadInt(Attributes, TEXT("first"), Pair.First, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("second"), Pair.Second, Result, Context, Line, true);
		bValid &= ReadInt(Attributes, TEXT("amount"), Pair.Amount, Result, Context, Line, true);
		if (!bValid)
		{
			return;
		}

		AddKerningPair(Pair, Result, KerningIndices, Line);
	}

	bool ParseTagLine(
		const FString& Line,
		FString& OutTag,
		FBMFontAttributes& OutAttributes,
		FString& OutError)
	{
		OutTag.Reset();
		OutAttributes.Reset();
		OutError.Reset();

		int32 Index = 0;
		while (Index < Line.Len() && FChar::IsWhitespace(Line[Index]))
		{
			++Index;
		}

		const int32 TagStart = Index;
		while (Index < Line.Len() && !FChar::IsWhitespace(Line[Index]))
		{
			++Index;
		}
		OutTag = Line.Mid(TagStart, Index - TagStart);
		if (OutTag.IsEmpty())
		{
			return true;
		}

		while (Index < Line.Len())
		{
			while (Index < Line.Len() && FChar::IsWhitespace(Line[Index]))
			{
				++Index;
			}
			if (Index >= Line.Len())
			{
				break;
			}

			const int32 KeyStart = Index;
			while (Index < Line.Len() && Line[Index] != TEXT('=') && !FChar::IsWhitespace(Line[Index]))
			{
				++Index;
			}
			const FString Key = Line.Mid(KeyStart, Index - KeyStart);
			while (Index < Line.Len() && FChar::IsWhitespace(Line[Index]))
			{
				++Index;
			}
			if (Key.IsEmpty() || Index >= Line.Len() || Line[Index] != TEXT('='))
			{
				OutError = FString::Printf(TEXT("Malformed attribute near '%s'."), *Line.Mid(KeyStart));
				return false;
			}
			++Index;
			while (Index < Line.Len() && FChar::IsWhitespace(Line[Index]))
			{
				++Index;
			}

			FString Value;
			if (Index < Line.Len() && Line[Index] == TEXT('"'))
			{
				++Index;
				const int32 ValueStart = Index;
				while (Index < Line.Len() && Line[Index] != TEXT('"'))
				{
					++Index;
				}
				if (Index >= Line.Len())
				{
					OutError = FString::Printf(TEXT("Unterminated quoted value for '%s'."), *Key);
					return false;
				}
				Value = Line.Mid(ValueStart, Index - ValueStart);
				++Index;
			}
			else
			{
				const int32 ValueStart = Index;
				while (Index < Line.Len() && !FChar::IsWhitespace(Line[Index]))
				{
					++Index;
				}
				Value = Line.Mid(ValueStart, Index - ValueStart);
			}

			OutAttributes.Add(Key, MoveTemp(Value));
		}

		return true;
	}

	FBMFontAttributes GetXmlAttributes(const FXmlNode& Node)
	{
		FBMFontAttributes Attributes;
		for (const FXmlAttribute& Attribute : Node.GetAttributes())
		{
			Attributes.Add(Attribute.GetTag(), Attribute.GetValue());
		}
		return Attributes;
	}

	bool IsUnicodeScalarValue(const int32 Codepoint)
	{
		return Codepoint >= 0
			&& Codepoint <= 0x10FFFF
			&& !(Codepoint >= 0xD800 && Codepoint <= 0xDFFF);
	}

	void ValidateData(FBMFontParseResult& Result)
	{
		const FBMFontCommon& Common = Result.Data.Common;
		if (Common.LineHeight <= 0)
		{
			AddError(Result, TEXT("common.lineHeight must be greater than zero."));
		}
		if (Common.ScaleWidth <= 0 || Common.ScaleHeight <= 0)
		{
			AddError(Result, TEXT("common.scaleW and common.scaleH must be greater than zero."));
		}
		if (Common.PageCount <= 0)
		{
			AddError(Result, TEXT("common.pages must be greater than zero."));
		}
		if (Common.PageCount != Result.Data.Pages.Num())
		{
			AddError(
				Result,
				FString::Printf(
					TEXT("common.pages declares %d page(s), but %d page record(s) were parsed."),
					Common.PageCount,
					Result.Data.Pages.Num()
				)
			);
		}
		if (Result.Data.Glyphs.IsEmpty())
		{
			AddError(Result, TEXT("The descriptor contains no glyph records."));
		}

		TSet<int32> PageIds;
		for (const FBMFontPage& Page : Result.Data.Pages)
		{
			PageIds.Add(Page.Id);
			if (Page.File.IsEmpty())
			{
				AddError(Result, FString::Printf(TEXT("Page %d has an empty file name."), Page.Id));
			}
		}

		for (const TPair<int32, FBMFontGlyph>& Entry : Result.Data.Glyphs)
		{
			const FBMFontGlyph& Glyph = Entry.Value;
			if (!IsUnicodeScalarValue(Glyph.Codepoint))
			{
				AddError(Result, FString::Printf(TEXT("Glyph id %d is not a valid Unicode scalar value."), Glyph.Codepoint));
			}
			if (Glyph.X < 0 || Glyph.Y < 0 || Glyph.Width < 0 || Glyph.Height < 0)
			{
				AddError(Result, FString::Printf(TEXT("Glyph %d has a negative atlas rectangle."), Glyph.Codepoint));
			}
			if (Common.ScaleWidth > 0 && Common.ScaleHeight > 0
				&& (Glyph.X + Glyph.Width > Common.ScaleWidth || Glyph.Y + Glyph.Height > Common.ScaleHeight))
			{
				AddError(Result, FString::Printf(TEXT("Glyph %d lies outside the declared atlas size."), Glyph.Codepoint));
			}
			if (!PageIds.Contains(Glyph.Page))
			{
				AddError(
					Result,
					FString::Printf(TEXT("Glyph %d references missing page id %d."), Glyph.Codepoint, Glyph.Page)
				);
			}
		}

		if (Common.bPacked)
		{
			AddWarning(
				Result,
				TEXT("The descriptor uses packed texture channels. Glyphs render through the channel-extraction material; outline channels are not composited separately.")
			);
		}
	}

	class FBMFontByteReader
	{
	public:
		explicit FBMFontByteReader(const TArrayView<const uint8> InBytes)
			: Bytes(InBytes)
		{
		}

		int32 Remaining() const
		{
			return Bytes.Num() - Offset;
		}

		int32 Tell() const
		{
			return Offset;
		}

		bool ReadUInt8(uint8& OutValue)
		{
			if (Remaining() < 1)
			{
				return false;
			}
			OutValue = Bytes[Offset++];
			return true;
		}

		bool ReadUInt16(uint16& OutValue)
		{
			if (Remaining() < 2)
			{
				return false;
			}
			OutValue = static_cast<uint16>(Bytes[Offset])
				| (static_cast<uint16>(Bytes[Offset + 1]) << 8);
			Offset += 2;
			return true;
		}

		bool ReadInt16(int16& OutValue)
		{
			uint16 Raw = 0;
			if (!ReadUInt16(Raw))
			{
				return false;
			}
			OutValue = static_cast<int16>(Raw);
			return true;
		}

		bool ReadUInt32(uint32& OutValue)
		{
			if (Remaining() < 4)
			{
				return false;
			}
			OutValue = static_cast<uint32>(Bytes[Offset])
				| (static_cast<uint32>(Bytes[Offset + 1]) << 8)
				| (static_cast<uint32>(Bytes[Offset + 2]) << 16)
				| (static_cast<uint32>(Bytes[Offset + 3]) << 24);
			Offset += 4;
			return true;
		}

		bool ReadUtf8String(FString& OutValue)
		{
			const int32 Start = Offset;
			while (Offset < Bytes.Num() && Bytes[Offset] != 0)
			{
				++Offset;
			}
			if (Offset >= Bytes.Num())
			{
				return false;
			}

			const int32 Length = Offset - Start;
			const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + Start), Length);
			OutValue = FString(Converted.Length(), Converted.Get());
			++Offset;
			return true;
		}

	private:
		TArrayView<const uint8> Bytes;
		int32 Offset = 0;
	};

	void ParseBinaryInfo(FBMFontByteReader& Reader, FBMFontParseResult& Result)
	{
		int16 FontSize = 0;
		uint8 BitField = 0;
		uint8 CharacterSet = 0;
		uint16 StretchHeight = 0;
		uint8 Supersampling = 0;
		uint8 PaddingUp = 0;
		uint8 PaddingRight = 0;
		uint8 PaddingDown = 0;
		uint8 PaddingLeft = 0;
		uint8 SpacingHorizontal = 0;
		uint8 SpacingVertical = 0;
		uint8 Outline = 0;

		if (!Reader.ReadInt16(FontSize)
			|| !Reader.ReadUInt8(BitField)
			|| !Reader.ReadUInt8(CharacterSet)
			|| !Reader.ReadUInt16(StretchHeight)
			|| !Reader.ReadUInt8(Supersampling)
			|| !Reader.ReadUInt8(PaddingUp)
			|| !Reader.ReadUInt8(PaddingRight)
			|| !Reader.ReadUInt8(PaddingDown)
			|| !Reader.ReadUInt8(PaddingLeft)
			|| !Reader.ReadUInt8(SpacingHorizontal)
			|| !Reader.ReadUInt8(SpacingVertical)
			|| !Reader.ReadUInt8(Outline))
		{
			AddError(Result, TEXT("Binary info block is truncated."));
			return;
		}

		FBMFontInfo& Info = Result.Data.Info;
		Info.Size = FontSize;
		Info.bSmooth = (BitField & 0x01) != 0;
		Info.bUnicode = (BitField & 0x02) != 0;
		Info.bItalic = (BitField & 0x04) != 0;
		Info.bBold = (BitField & 0x08) != 0;
		Info.bFixedHeight = (BitField & 0x10) != 0;
		Info.CharacterSetId = CharacterSet;
		Info.StretchHeight = StretchHeight;
		Info.Supersampling = Supersampling;
		Info.PaddingUp = PaddingUp;
		Info.PaddingRight = PaddingRight;
		Info.PaddingDown = PaddingDown;
		Info.PaddingLeft = PaddingLeft;
		Info.Spacing = FIntPoint(SpacingHorizontal, SpacingVertical);
		Info.Outline = Outline;
		if (!Reader.ReadUtf8String(Info.Face))
		{
			AddError(Result, TEXT("Binary info block has an unterminated font name."));
		}
	}

	void ParseBinaryCommon(FBMFontByteReader& Reader, FBMFontParseResult& Result)
	{
		uint16 LineHeight = 0;
		uint16 Base = 0;
		uint16 ScaleWidth = 0;
		uint16 ScaleHeight = 0;
		uint16 PageCount = 0;
		uint8 BitField = 0;
		uint8 AlphaChannel = 0;
		uint8 RedChannel = 0;
		uint8 GreenChannel = 0;
		uint8 BlueChannel = 0;

		if (!Reader.ReadUInt16(LineHeight)
			|| !Reader.ReadUInt16(Base)
			|| !Reader.ReadUInt16(ScaleWidth)
			|| !Reader.ReadUInt16(ScaleHeight)
			|| !Reader.ReadUInt16(PageCount)
			|| !Reader.ReadUInt8(BitField)
			|| !Reader.ReadUInt8(AlphaChannel)
			|| !Reader.ReadUInt8(RedChannel)
			|| !Reader.ReadUInt8(GreenChannel)
			|| !Reader.ReadUInt8(BlueChannel))
		{
			AddError(Result, TEXT("Binary common block is truncated."));
			return;
		}

		FBMFontCommon& Common = Result.Data.Common;
		Common.LineHeight = LineHeight;
		Common.Base = Base;
		Common.ScaleWidth = ScaleWidth;
		Common.ScaleHeight = ScaleHeight;
		Common.PageCount = PageCount;
		Common.bPacked = (BitField & 0x80) != 0;
		Common.AlphaChannel = ToChannelContent(AlphaChannel);
		Common.RedChannel = ToChannelContent(RedChannel);
		Common.GreenChannel = ToChannelContent(GreenChannel);
		Common.BlueChannel = ToChannelContent(BlueChannel);
	}

	void ParseBinaryPages(FBMFontByteReader& Reader, FBMFontParseResult& Result)
	{
		int32 PageId = 0;
		while (Reader.Remaining() > 0)
		{
			FBMFontPage Page;
			Page.Id = PageId++;
			if (!Reader.ReadUtf8String(Page.File))
			{
				AddError(Result, TEXT("Binary pages block contains an unterminated file name."));
				return;
			}
			Result.Data.Pages.Add(MoveTemp(Page));
		}
	}

	void ParseBinaryGlyphs(FBMFontByteReader& Reader, FBMFontParseResult& Result)
	{
		if (Reader.Remaining() % 20 != 0)
		{
			AddError(Result, TEXT("Binary chars block size is not divisible by 20."));
			return;
		}

		while (Reader.Remaining() >= 20)
		{
			uint32 Codepoint = 0;
			uint16 X = 0;
			uint16 Y = 0;
			uint16 Width = 0;
			uint16 Height = 0;
			int16 XOffset = 0;
			int16 YOffset = 0;
			int16 XAdvance = 0;
			uint8 Page = 0;
			uint8 Channel = 0;
			Reader.ReadUInt32(Codepoint);
			Reader.ReadUInt16(X);
			Reader.ReadUInt16(Y);
			Reader.ReadUInt16(Width);
			Reader.ReadUInt16(Height);
			Reader.ReadInt16(XOffset);
			Reader.ReadInt16(YOffset);
			Reader.ReadInt16(XAdvance);
			Reader.ReadUInt8(Page);
			Reader.ReadUInt8(Channel);

			FBMFontGlyph Glyph;
			Glyph.Codepoint = static_cast<int32>(Codepoint);
			Glyph.X = X;
			Glyph.Y = Y;
			Glyph.Width = Width;
			Glyph.Height = Height;
			Glyph.XOffset = XOffset;
			Glyph.YOffset = YOffset;
			Glyph.XAdvance = XAdvance;
			Glyph.Page = Page;
			Glyph.Channel = Channel;
			if (Result.Data.Glyphs.Contains(Glyph.Codepoint))
			{
				AddWarning(Result, FString::Printf(TEXT("Duplicate binary glyph id %d; the last record wins."), Glyph.Codepoint));
			}
			Result.Data.Glyphs.Add(Glyph.Codepoint, Glyph);
		}
	}

	void ParseBinaryKernings(
		FBMFontByteReader& Reader,
		FBMFontParseResult& Result,
		TMap<uint64, int32>& KerningIndices)
	{
		if (Reader.Remaining() % 10 != 0)
		{
			AddError(Result, TEXT("Binary kerning block size is not divisible by 10."));
			return;
		}

		while (Reader.Remaining() >= 10)
		{
			uint32 First = 0;
			uint32 Second = 0;
			int16 Amount = 0;
			Reader.ReadUInt32(First);
			Reader.ReadUInt32(Second);
			Reader.ReadInt16(Amount);

			FBMFontKerningPair Pair;
			Pair.First = static_cast<int32>(First);
			Pair.Second = static_cast<int32>(Second);
			Pair.Amount = Amount;
			AddKerningPair(Pair, Result, KerningIndices);
		}
	}
}

bool FBMFontParseResult::HasErrors() const
{
	return Messages.ContainsByPredicate(
		[](const FBMFontParseMessage& Message)
		{
			return Message.Severity == EBMFontParseMessageSeverity::Error;
		}
	);
}

bool FBMFontParseResult::IsSuccess() const
{
	return !HasErrors() && Data.IsValid();
}

FBMFontParseResult FBMFontParser::Parse(const TArrayView<const uint8> Bytes)
{
	if (Bytes.Num() >= 4
		&& Bytes[0] == static_cast<uint8>('B')
		&& Bytes[1] == static_cast<uint8>('M')
		&& Bytes[2] == static_cast<uint8>('F'))
	{
		return ParseBinary(Bytes);
	}

	FBMFontParseResult Result;
	if (Bytes.IsEmpty())
	{
		AddError(Result, TEXT("The BMFont descriptor is empty."));
		return Result;
	}

	FString DescriptorText;
	FFileHelper::BufferToString(DescriptorText, Bytes.GetData(), Bytes.Num());
	FString Trimmed = DescriptorText;
	Trimmed.TrimStartInline();
	if (Trimmed.StartsWith(TEXT("<")))
	{
		return ParseXml(DescriptorText);
	}

	return ParseText(DescriptorText);
}

FBMFontParseResult FBMFontParser::ParseText(const FString& DescriptorText)
{
	FBMFontParseResult Result;
	Result.Data.DescriptorFormat = EBMFontDescriptorFormat::Text;

	FString Normalized = DescriptorText;
	if (!Normalized.IsEmpty() && Normalized[0] == 0xFEFF)
	{
		Normalized.RemoveAt(0);
	}

	TArray<FString> Lines;
	Normalized.ParseIntoArrayLines(Lines, false);
	int32 ExpectedGlyphCount = INDEX_NONE;
	int32 ExpectedKerningCount = INDEX_NONE;
	TMap<uint64, int32> KerningIndices;

	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		FString Line = Lines[LineIndex];
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}

		FString Tag;
		FString ParseError;
		FBMFontAttributes Attributes;
		if (!ParseTagLine(Line, Tag, Attributes, ParseError))
		{
			AddError(Result, MoveTemp(ParseError), LineIndex + 1);
			continue;
		}

		if (Tag == TEXT("info"))
		{
			ParseInfoAttributes(Attributes, Result, LineIndex + 1);
		}
		else if (Tag == TEXT("common"))
		{
			ParseCommonAttributes(Attributes, Result, LineIndex + 1);
		}
		else if (Tag == TEXT("page"))
		{
			ParsePageAttributes(Attributes, Result, LineIndex + 1);
		}
		else if (Tag == TEXT("chars"))
		{
			ReadInt(Attributes, TEXT("count"), ExpectedGlyphCount, Result, TEXT("chars record"), LineIndex + 1, false);
		}
		else if (Tag == TEXT("char"))
		{
			ParseGlyphAttributes(Attributes, Result, LineIndex + 1);
		}
		else if (Tag == TEXT("kernings"))
		{
			ReadInt(Attributes, TEXT("count"), ExpectedKerningCount, Result, TEXT("kernings record"), LineIndex + 1, false);
		}
		else if (Tag == TEXT("kerning"))
		{
			ParseKerningAttributes(Attributes, Result, KerningIndices, LineIndex + 1);
		}
		else
		{
			AddWarning(Result, FString::Printf(TEXT("Unknown BMFont record '%s' was ignored."), *Tag), LineIndex + 1);
		}
	}

	if (ExpectedGlyphCount != INDEX_NONE && ExpectedGlyphCount != Result.Data.Glyphs.Num())
	{
		AddWarning(
			Result,
			FString::Printf(
				TEXT("chars.count declares %d glyph(s), but %d were parsed."),
				ExpectedGlyphCount,
				Result.Data.Glyphs.Num()
			)
		);
	}
	if (ExpectedKerningCount != INDEX_NONE && ExpectedKerningCount != Result.Data.KerningPairs.Num())
	{
		AddWarning(
			Result,
			FString::Printf(
				TEXT("kernings.count declares %d pair(s), but %d were parsed."),
				ExpectedKerningCount,
				Result.Data.KerningPairs.Num()
			)
		);
	}

	ValidateData(Result);
	return Result;
}

FBMFontParseResult FBMFontParser::ParseXml(const FString& DescriptorXml)
{
	FBMFontParseResult Result;
	Result.Data.DescriptorFormat = EBMFontDescriptorFormat::Xml;

	// FXmlFile removes an entire line when that line starts with an XML declaration.
	// A valid compact BMFont document may place the declaration and root element on
	// the same line, so strip the declaration before handing the buffer to FXmlFile.
	FString NormalizedXml = DescriptorXml;
	if (!NormalizedXml.IsEmpty() && NormalizedXml[0] == 0xFEFF)
	{
		NormalizedXml.RemoveAt(0);
	}
	NormalizedXml.TrimStartInline();
	if (NormalizedXml.StartsWith(TEXT("<?xml"), ESearchCase::IgnoreCase))
	{
		const int32 DeclarationEnd = NormalizedXml.Find(TEXT("?>"));
		if (DeclarationEnd == INDEX_NONE)
		{
			AddError(Result, TEXT("Invalid BMFont XML: unterminated XML declaration."));
			return Result;
		}
		NormalizedXml.RightChopInline(DeclarationEnd + 2);
	}

	FXmlFile XmlFile(NormalizedXml, EConstructMethod::ConstructFromBuffer);
	if (!XmlFile.IsValid())
	{
		AddError(Result, FString::Printf(TEXT("Invalid BMFont XML: %s"), *XmlFile.GetLastError()));
		return Result;
	}

	const FXmlNode* Root = XmlFile.GetRootNode();
	if (Root == nullptr || Root->GetTag() != TEXT("font"))
	{
		AddError(Result, TEXT("BMFont XML root element must be <font>."));
		return Result;
	}

	if (const FXmlNode* InfoNode = Root->FindChildNode(TEXT("info")))
	{
		ParseInfoAttributes(GetXmlAttributes(*InfoNode), Result, INDEX_NONE);
	}
	if (const FXmlNode* CommonNode = Root->FindChildNode(TEXT("common")))
	{
		ParseCommonAttributes(GetXmlAttributes(*CommonNode), Result, INDEX_NONE);
	}
	else
	{
		AddError(Result, TEXT("BMFont XML is missing the <common> element."));
	}

	if (const FXmlNode* PagesNode = Root->FindChildNode(TEXT("pages")))
	{
		for (const FXmlNode* PageNode : PagesNode->GetChildrenNodes())
		{
			if (PageNode != nullptr && PageNode->GetTag() == TEXT("page"))
			{
				ParsePageAttributes(GetXmlAttributes(*PageNode), Result, INDEX_NONE);
			}
		}
	}

	int32 ExpectedGlyphCount = INDEX_NONE;
	if (const FXmlNode* CharsNode = Root->FindChildNode(TEXT("chars")))
	{
		const FBMFontAttributes Attributes = GetXmlAttributes(*CharsNode);
		ReadInt(Attributes, TEXT("count"), ExpectedGlyphCount, Result, TEXT("chars element"), INDEX_NONE, false);
		for (const FXmlNode* CharNode : CharsNode->GetChildrenNodes())
		{
			if (CharNode != nullptr && CharNode->GetTag() == TEXT("char"))
			{
				ParseGlyphAttributes(GetXmlAttributes(*CharNode), Result, INDEX_NONE);
			}
		}
	}

	int32 ExpectedKerningCount = INDEX_NONE;
	TMap<uint64, int32> KerningIndices;
	if (const FXmlNode* KerningsNode = Root->FindChildNode(TEXT("kernings")))
	{
		const FBMFontAttributes Attributes = GetXmlAttributes(*KerningsNode);
		ReadInt(Attributes, TEXT("count"), ExpectedKerningCount, Result, TEXT("kernings element"), INDEX_NONE, false);
		for (const FXmlNode* KerningNode : KerningsNode->GetChildrenNodes())
		{
			if (KerningNode != nullptr && KerningNode->GetTag() == TEXT("kerning"))
			{
				ParseKerningAttributes(GetXmlAttributes(*KerningNode), Result, KerningIndices, INDEX_NONE);
			}
		}
	}

	if (ExpectedGlyphCount != INDEX_NONE && ExpectedGlyphCount != Result.Data.Glyphs.Num())
	{
		AddWarning(Result, TEXT("The XML chars.count value does not match the number of parsed glyphs."));
	}
	if (ExpectedKerningCount != INDEX_NONE && ExpectedKerningCount != Result.Data.KerningPairs.Num())
	{
		AddWarning(Result, TEXT("The XML kernings.count value does not match the number of parsed kerning pairs."));
	}

	ValidateData(Result);
	return Result;
}

FBMFontParseResult FBMFontParser::ParseBinary(const TArrayView<const uint8> Bytes)
{
	FBMFontParseResult Result;
	Result.Data.DescriptorFormat = EBMFontDescriptorFormat::Binary;
	if (Bytes.Num() < 4
		|| Bytes[0] != static_cast<uint8>('B')
		|| Bytes[1] != static_cast<uint8>('M')
		|| Bytes[2] != static_cast<uint8>('F'))
	{
		AddError(Result, TEXT("Binary BMFont descriptor is missing the BMF header."));
		return Result;
	}
	if (Bytes[3] != 3)
	{
		AddError(Result, FString::Printf(TEXT("Unsupported binary BMFont version %d; version 3 is required."), Bytes[3]));
		return Result;
	}

	TMap<uint64, int32> KerningIndices;
	int32 Offset = 4;
	while (Offset < Bytes.Num())
	{
		if (Bytes.Num() - Offset < 5)
		{
			AddError(Result, TEXT("Binary BMFont block header is truncated."));
			break;
		}

		const uint8 BlockType = Bytes[Offset++];
		const uint32 BlockSize = static_cast<uint32>(Bytes[Offset])
			| (static_cast<uint32>(Bytes[Offset + 1]) << 8)
			| (static_cast<uint32>(Bytes[Offset + 2]) << 16)
			| (static_cast<uint32>(Bytes[Offset + 3]) << 24);
		Offset += 4;
		if (BlockSize > static_cast<uint32>(Bytes.Num() - Offset))
		{
			AddError(Result, FString::Printf(TEXT("Binary BMFont block %d extends past the end of the file."), BlockType));
			break;
		}

		FBMFontByteReader Reader(Bytes.Slice(Offset, static_cast<int32>(BlockSize)));
		switch (BlockType)
		{
		case 1:
			ParseBinaryInfo(Reader, Result);
			break;
		case 2:
			ParseBinaryCommon(Reader, Result);
			break;
		case 3:
			ParseBinaryPages(Reader, Result);
			break;
		case 4:
			ParseBinaryGlyphs(Reader, Result);
			break;
		case 5:
			ParseBinaryKernings(Reader, Result, KerningIndices);
			break;
		default:
			AddWarning(Result, FString::Printf(TEXT("Unknown binary BMFont block type %d was ignored."), BlockType));
			break;
		}

		Offset += static_cast<int32>(BlockSize);
	}

	ValidateData(Result);
	return Result;
}
