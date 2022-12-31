/*
Copyright (c) 2005-2018, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    MXF.h
    \version $Id$
    \brief   MXF objects
*/

#ifndef _MXF_H_
#define _MXF_H_

#include "MXFTypes.h"
#include <algorithm>

namespace ASDCP
{
  namespace MXF
    {
      class InterchangeObject;

      const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

      //
      typedef ASDCP::MXF::InterchangeObject* (*MXFObjectFactory_t)(const Dictionary*&);

      //
      void SetObjectFactory(const UL& label, MXFObjectFactory_t factory);

      //
      InterchangeObject* CreateObject(const Dictionary*& Dict, const UL& label);


      // seek an open file handle to the start of the RIP KLV packet
      Result_t SeekToRIP(const Kumu::FileReader&);
      
      //
      class RIP : public ASDCP::KLVFilePacket
	{
	  ASDCP_NO_COPY_CONSTRUCT(RIP);
	  RIP();

	public:
	  //
	  class PartitionPair : public Kumu::IArchive
	    {
	    public:
	      ui32_t BodySID;
	      ui64_t ByteOffset;

	      PartitionPair() : BodySID(0), ByteOffset(0) {}
	      PartitionPair(ui32_t sid, ui64_t offset) : BodySID(sid), ByteOffset(offset) {}
	      virtual ~PartitionPair() {}

	      ui32_t Size() { return sizeof(ui32_t) + sizeof(ui64_t); }

	      inline const char* EncodeString(char* str_buf, ui32_t buf_len) const {
		Kumu::ui64Printer offset_str(ByteOffset);
		snprintf(str_buf, buf_len, "%-6u: %s", BodySID, offset_str.c_str());
		return str_buf;
	      }

	      inline bool HasValue() const { return true; }
	      inline ui32_t ArchiveLength() const { return sizeof(ui32_t) + sizeof(ui64_t); }

	      inline bool Unarchive(Kumu::MemIOReader* Reader) {
		if ( ! Reader->ReadUi32BE(&BodySID) ) return false;
		if ( ! Reader->ReadUi64BE(&ByteOffset) ) return false;
		return true;
	      }
	      
	      inline bool Archive(Kumu::MemIOWriter* Writer) const {
		if ( ! Writer->WriteUi32BE(BodySID) ) return false;
		if ( ! Writer->WriteUi64BE(ByteOffset) ) return false;
		return true;
	      }
	    };

	  const Dictionary*& m_Dict;

	  typedef SimpleArray<PartitionPair>::iterator pair_iterator;
	  typedef SimpleArray<PartitionPair>::const_iterator const_pair_iterator;

	  SimpleArray<PartitionPair> PairArray;

	RIP(const Dictionary*& d) : m_Dict(d) {}
	  virtual ~RIP() {}
	  virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
	  virtual Result_t WriteToFile(Kumu::FileWriter& Writer);
	  virtual bool GetPairBySID(ui32_t, PartitionPair&) const;
	  virtual void     Dump(FILE* = 0);
	};


      //
      class Partition : public ASDCP::KLVFilePacket
	{
	  ASDCP_NO_COPY_CONSTRUCT(Partition);
	  Partition();

	protected:
	  class PacketList
	  {
	  public:
	    std::list<InterchangeObject*> m_List;
	    std::map<UUID, InterchangeObject*> m_Map;

	    ~PacketList();
	    void AddPacket(InterchangeObject* ThePacket); // takes ownership
	    Result_t GetMDObjectByID(const UUID& ObjectID, InterchangeObject** Object);
	    Result_t GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object);
	    Result_t GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList);
	  };

	  mem_ptr<PacketList> m_PacketList;

	public:
	  const Dictionary*& m_Dict;

	  ui16_t    MajorVersion;
	  ui16_t    MinorVersion;
	  ui32_t    KAGSize;
	  ui64_t    ThisPartition;
	  ui64_t    PreviousPartition;
	  ui64_t    FooterPartition;
	  ui64_t    HeaderByteCount;
	  ui64_t    IndexByteCount;
	  ui32_t    IndexSID;
	  ui64_t    BodyOffset;
	  ui32_t    BodySID;
	  UL        OperationalPattern;
	  Batch<UL> EssenceContainers;

	  Partition(const Dictionary*&);
	  virtual ~Partition();
	  virtual void     AddChildObject(InterchangeObject*); // takes ownership
	  virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToFile(Kumu::FileWriter& Writer, UL& PartitionLabel);
	  virtual ui32_t   ArchiveSize(); // returns the size of the archived structure
	  virtual void     Dump(FILE* = 0);
	};


      //
      class Primer : public ASDCP::KLVFilePacket, public ASDCP::IPrimerLookup
	{
	  class h__PrimerLookup;
	  mem_ptr<h__PrimerLookup> m_Lookup;
	  ui8_t   m_LocalTag;
	  ASDCP_NO_COPY_CONSTRUCT(Primer);
	  Primer();

	public:
	  //
	class LocalTagEntry : Kumu::IArchive
	    {
	    public:
	      TagValue    Tag;
	      ASDCP::UL   UL;

	      LocalTagEntry() { Tag.a = Tag.b = 0; }
	    LocalTagEntry(const TagValue& tag, ASDCP::UL& ul) : Tag(tag), UL(ul) {}

	      bool operator<(const LocalTagEntry& rhs) const {
		if ( Tag.a < rhs.Tag.a )
		  {
		    return true;
		  }
		
		if ( Tag.a == rhs.Tag.a && Tag.b < rhs.Tag.b )
		  {
		    return true;
		  }

		return false;
	      }

	      inline const char* EncodeString(char* str_buf, ui32_t buf_len) const {
		snprintf(str_buf, buf_len, "%02x %02x: ", Tag.a, Tag.b);
		UL.EncodeString(str_buf + strlen(str_buf), buf_len - (ui32_t)strlen(str_buf));
		return str_buf;
	      }

	      inline bool HasValue() const { return UL.HasValue(); }
	      inline ui32_t ArchiveLength() const { return 2 + UL.ArchiveLength(); }

	      inline bool Unarchive(Kumu::MemIOReader* Reader) {
		if ( ! Reader->ReadUi8(&Tag.a) ) return false;
		if ( ! Reader->ReadUi8(&Tag.b) ) return false;
		return UL.Unarchive(Reader);
	      }

	      inline bool Archive(Kumu::MemIOWriter* Writer) const {
		if ( ! Writer->WriteUi8(Tag.a) ) return false;
		if ( ! Writer->WriteUi8(Tag.b) ) return false;
		return UL.Archive(Writer);
	      }
	    };

	  Batch<LocalTagEntry> LocalTagEntryBatch;
	  const Dictionary*& m_Dict;

	  Primer(const Dictionary*&);
	  virtual ~Primer();

	  virtual void     ClearTagList();
	  virtual Result_t InsertTag(const MDDEntry& Entry, ASDCP::TagValue& Tag);
	  virtual Result_t TagForKey(const ASDCP::UL& Key, ASDCP::TagValue& Tag);

          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
          virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual Result_t WriteToFile(Kumu::FileWriter& Writer);
	  virtual void     Dump(FILE* = 0);
	};

      // wrapper object manages optional properties
      template <class PropertyType>
	class optional_property
	{
	  PropertyType m_property;
	  bool m_has_value;

	public:
	optional_property() : m_has_value(false) {}
	optional_property(const PropertyType& value) : m_property(value), m_has_value(true) {}
	  const optional_property<PropertyType>& operator=(const PropertyType& rhs) {
	    this->m_property = rhs;
	    this->m_has_value = true;
	    return *this;
	  }
	  bool operator==(const PropertyType& rhs) const { return this->m_property == rhs; }
	  bool operator==(const optional_property<PropertyType>& rhs) const { return this->m_property == rhs.m_property; }
	  operator PropertyType&() { return this->m_property; }
	  void set(const PropertyType& rhs) { this->m_property = rhs; this->m_has_value = true; }
	  void set_has_value(bool has_value = true) { this->m_has_value = has_value; }
	  void reset(const PropertyType& rhs) { this->m_has_value = false; }
	  bool empty() const { return ! m_has_value; }
	  PropertyType& get() { return m_property; }
	  const PropertyType& const_get() const { return m_property; }
	};

      // wrapper object manages optional properties
      template <class PropertyType>
	class optional_container_property
	{
	  PropertyType m_property;

	public:
	  optional_container_property() {}
	optional_container_property(const PropertyType& value) : m_property(value) {}
	  const optional_container_property<PropertyType>& operator=(const PropertyType& rhs) {
	    this->Copy(rhs.m_property);
	    return *this;
	  }

	  bool operator==(const PropertyType& rhs) const { return this->m_property == rhs; }
	  bool operator==(const optional_property<PropertyType>& rhs) const { return this->m_property == rhs.m_property; }
	  operator PropertyType&() { return this->m_property; }
	  void set(const PropertyType& rhs) { this->m_property = rhs; }
	  void reset(const PropertyType& rhs) { this->clear(); }
	  bool empty() const { return ! this->m_property.HasValue(); }
	  PropertyType& get() { return m_property; }
	  const PropertyType& const_get() const { return m_property; }
	};

      // base class of all metadata objects
      //
      class InterchangeObject : public ASDCP::KLVPacket
	{
	  InterchangeObject();

	public:
	  const Dictionary*& m_Dict;
	  IPrimerLookup* m_Lookup;
	  UUID           InstanceUID;
	  optional_property<UUID>  GenerationUID;

	InterchangeObject(const Dictionary*& d) : m_Dict(d), m_Lookup(0) {}
	  virtual ~InterchangeObject() {}

	  virtual void Copy(const InterchangeObject& rhs);
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual bool     IsA(const byte_t* label);
	  virtual const char* ObjectName() { return "InterchangeObject"; }
	  virtual void     Dump(FILE* stream = 0);
	};

      //
      typedef std::list<InterchangeObject*> InterchangeObject_list_t;

      //
      class Preface : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Preface);
	  Preface();

	public:
	  const Dictionary*& m_Dict;
	  Kumu::Timestamp    LastModifiedDate;
	  ui16_t       Version;
	  optional_property<ui32_t> ObjectModelVersion;
	  optional_property<UUID> PrimaryPackage;
	  Array<UUID>  Identifications;
	  UUID         ContentStorage;
	  UL           OperationalPattern;
	  Batch<UL>    EssenceContainers;
	  Batch<UL>    DMSchemes;
	  optional_property<Batch<UL> > ApplicationSchemes;
	  optional_property<Batch<UL> > ConformsToSpecifications;

	  Preface(const Dictionary*& d);
	  virtual ~Preface() {}

	  virtual void Copy(const Preface& rhs);
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};

      const ui32_t MaxIndexSegmentSize = 65536;

      //
      class IndexTableSegment : public InterchangeObject
	{
	  IndexTableSegment();
	  ASDCP_NO_COPY_CONSTRUCT(IndexTableSegment);

	public:
	  //
	class DeltaEntry : public Kumu::IArchive
	    {
	    public:
	      i8_t    PosTableIndex;
	      ui8_t   Slice;
	      ui32_t  ElementData;

	      DeltaEntry() : PosTableIndex(0), Slice(0), ElementData(0) {}
	      DeltaEntry(i8_t pos, ui8_t slice, ui32_t data) : PosTableIndex(pos), Slice(slice), ElementData(data) {}
	      inline bool HasValue() const { return true; }
	      ui32_t      ArchiveLength() const { return sizeof(ui32_t) + 2; }
	      bool        Unarchive(Kumu::MemIOReader* Reader);
	      bool        Archive(Kumu::MemIOWriter* Writer) const;
	      const char* EncodeString(char* str_buf, ui32_t buf_len) const;
	    };

	  //
	  class IndexEntry : public Kumu::IArchive
	    {
	    public:
	      i8_t               TemporalOffset;
	      i8_t               KeyFrameOffset;
	      ui8_t              Flags;
	      ui64_t             StreamOffset;

	      // if you use these, you will need to change CBRIndexEntriesPerSegment in MXF.cpp
	      // to a more suitable value
	      //	      std::list<ui32_t>  SliceOffset;
	      //	      Array<Rational>    PosTable;

	      IndexEntry() : TemporalOffset(0), KeyFrameOffset(0), Flags(0), StreamOffset(0) {}
	      IndexEntry(i8_t t_ofst, i8_t k_ofst, ui8_t flags, ui64_t s_ofst) :
	            TemporalOffset(t_ofst), KeyFrameOffset(k_ofst), Flags(flags), StreamOffset(s_ofst) {}
	      inline bool HasValue() const { return true; }
	      ui32_t      ArchiveLength() const { return sizeof(ui64_t) + 3; };
	      bool        Unarchive(Kumu::MemIOReader* Reader);
	      bool        Archive(Kumu::MemIOWriter* Writer) const;
	      const char* EncodeString(char* str_buf, ui32_t buf_len) const;
	    };

	  const Dictionary*& m_Dict;
	  ui64_t  RtFileOffset; // not part of the MXF structure: used to manage runtime index access 
	  ui64_t  RtEntryOffset;

	  Rational    IndexEditRate;
	  ui64_t      IndexStartPosition;
	  ui64_t      IndexDuration;
	  ui32_t      EditUnitByteCount;
	  ui32_t      IndexSID;
	  ui32_t      BodySID;
	  ui8_t       SliceCount;
	  ui8_t       PosTableCount;
	  Array<DeltaEntry> DeltaEntryArray;
	  Array<IndexEntry> IndexEntryArray;

	  IndexTableSegment(const Dictionary*&);
	  virtual ~IndexTableSegment();

	  virtual void Copy(const IndexTableSegment& rhs);
	  virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};

      //---------------------------------------------------------------------------------
      //
      class Identification;
      class SourcePackage;

      //
      class OP1aHeader : public Partition
	{
	  Kumu::ByteString m_HeaderData;
	  ASDCP_NO_COPY_CONSTRUCT(OP1aHeader);
	  OP1aHeader();

	public:
	  const Dictionary*&  m_Dict;
	  ASDCP::MXF::Primer  m_Primer;
	  Preface*            m_Preface;

	  OP1aHeader(const Dictionary*&);
	  virtual ~OP1aHeader();
	  virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
	  virtual Result_t InitFromPartitionBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToFile(Kumu::FileWriter& Writer, ui32_t HeaderLength = 16384);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t GetMDObjectByID(const UUID&, InterchangeObject** = 0);
	  virtual Result_t GetMDObjectByType(const byte_t*, InterchangeObject** = 0);
	  virtual Result_t GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList);
	  Identification*  GetIdentification();
	  SourcePackage*   GetSourcePackage();
	};

      // Searches the header object and returns the edit rate based on the contents of the
      // File Package items.  Logs an error message and returns false if anthing goes wrong.
      bool GetEditRateFromFP(ASDCP::MXF::OP1aHeader& header, ASDCP::Rational& edit_rate);

      //
      class OPAtomIndexFooter : public Partition
	{
	  Kumu::ByteString    m_FooterData;
	  IndexTableSegment*  m_CurrentSegment;
	  ui32_t              m_BytesPerEditUnit;
	  Rational            m_EditRate;
	  ui32_t              m_BodySID;
	  IndexTableSegment::DeltaEntry m_DefaultDeltaEntry;

	  ASDCP_NO_COPY_CONSTRUCT(OPAtomIndexFooter);
	  OPAtomIndexFooter();

	public:
	  const Dictionary*&   m_Dict;
	  Kumu::fpos_t        m_ECOffset;
	  IPrimerLookup*      m_Lookup;
	 
	  OPAtomIndexFooter(const Dictionary*&);
	  virtual ~OPAtomIndexFooter();
	  virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
	  virtual Result_t InitFromPartitionBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToFile(Kumu::FileWriter& Writer, ui64_t duration);
	  virtual void     Dump(FILE* = 0);

	  virtual Result_t GetMDObjectByID(const UUID&, InterchangeObject** = 0);
	  virtual Result_t GetMDObjectByType(const byte_t*, InterchangeObject** = 0);
	  virtual Result_t GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList);

	  virtual Result_t Lookup(ui32_t frame_num, IndexTableSegment::IndexEntry&) const;
	  virtual void     PushIndexEntry(const IndexTableSegment::IndexEntry&);
	  virtual void     SetDeltaParams(const IndexTableSegment::DeltaEntry&);
	  virtual void     SetIndexParamsCBR(IPrimerLookup* lookup, ui32_t size, const Rational& Rate);
	  virtual void     SetIndexParamsVBR(IPrimerLookup* lookup, const Rational& Rate, Kumu::fpos_t offset);
	};

      //---------------------------------------------------------------------------------
      //

      //
      inline std::string to_lower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
      }

      // ignore case when searching for audio labels
      struct ci_comp
      {
	inline bool operator()(const std::string& a, const std::string& b) const {
	  return to_lower(a) < to_lower(b);
	}
      };

      struct label_traits
      {
        const std::string tag_name;
	const bool requires_prefix;
	const UL ul;

      label_traits(const std::string& tag_name, const bool requires_prefix, const UL ul) : 
	tag_name(tag_name), requires_prefix(requires_prefix), ul(ul) { }
      };

      typedef std::map<const std::string, const label_traits, ci_comp> mca_label_map_t;

      bool decode_mca_string(const std::string& s, const mca_label_map_t& labels,
			     const Dictionary*& dict, const std::string& language, InterchangeObject_list_t&, ui32_t&);

      //
      class ASDCP_MCAConfigParser : public InterchangeObject_list_t
	{
	  KM_NO_COPY_CONSTRUCT(ASDCP_MCAConfigParser);
	  ASDCP_MCAConfigParser();

	protected:
	  mca_label_map_t m_LabelMap;
	  ui32_t m_ChannelCount;
	  const Dictionary*& m_Dict;

	  
	public:
	  ASDCP_MCAConfigParser(const Dictionary*&);
	  bool DecodeString(const std::string& s, const std::string& language = "en-US");

	  // Valid only after a successful call to DecodeString
	  ui32_t ChannelCount() const;
	};

      //
      class AS02_MCAConfigParser : public ASDCP_MCAConfigParser
	{
	  KM_NO_COPY_CONSTRUCT(AS02_MCAConfigParser);
	  AS02_MCAConfigParser();
	  
	public:
	  AS02_MCAConfigParser(const Dictionary*&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _MXF_H_

//
// end MXF.h
//
