/*
Copyright (c) 2004-2016, John Hurst
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
/*! \file    AS_DCP_ATMOS.cpp
    \version $Id$
    \brief   AS-DCP library, Dolby Atmos essence reader and writer implementation
*/


#include <iostream>

#include "AS_DCP.h"
#include "AS_DCP_internal.h"

namespace ASDCP
{
namespace ATMOS
{
  static std::string ATMOS_PACKAGE_LABEL = "File Package: SMPTE-GC frame wrapping of Dolby ATMOS data";
  static std::string ATMOS_DEF_LABEL = "Dolby ATMOS Data Track";
  static byte_t ATMOS_ESSENCE_CODING[SMPTE_UL_LENGTH] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x05,
                                                          0x0e, 0x09, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00 };
} // namespace ATMOS
} // namespace ASDCP

//
std::ostream&
ASDCP::ATMOS::operator << (std::ostream& strm, const AtmosDescriptor& ADesc)
{
  char str_buf[40];
  strm << "        EditRate: " << ADesc.EditRate.Numerator << "/" << ADesc.EditRate.Denominator << std::endl;
  strm << " ContainerDuration: " << (unsigned) ADesc.ContainerDuration << std::endl;
  strm << " DataEssenceCoding: " << UL(ADesc.DataEssenceCoding).EncodeString(str_buf, 40) << std::endl;
  strm << "      AtmosVersion: " << (unsigned) ADesc.AtmosVersion << std::endl;
  strm << "   MaxChannelCount: " << (unsigned) ADesc.MaxChannelCount << std::endl;
  strm << "    MaxObjectCount: " << (unsigned) ADesc.MaxObjectCount << std::endl;
  strm << "           AtmosID: " << UUID(ADesc.AtmosID).EncodeString(str_buf, 40) << std::endl;
  strm << "        FirstFrame: " << (unsigned) ADesc.FirstFrame << std::endl;
  return strm;
}

//
void
ASDCP::ATMOS::AtmosDescriptorDump(const AtmosDescriptor& ADesc, FILE* stream)
{
  char str_buf[40];
  char atmosID_buf[40];
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
          EditRate: %d/%d\n\
   ContainerDuration: %u\n\
   DataEssenceCoding: %s\n\
        AtmosVersion: %u\n\
     MaxChannelCount: %u\n\
      MaxObjectCount: %u\n\
             AtmosID: %s\n\
           FirsFrame: %u\n",
          ADesc.EditRate.Numerator, ADesc.EditRate.Denominator,
          ADesc.ContainerDuration,
          UL(ADesc.DataEssenceCoding).EncodeString(str_buf, 40),
          ADesc.AtmosVersion,
          ADesc.MaxChannelCount,
          ADesc.MaxObjectCount,
          UUID(ADesc.AtmosID).EncodeString(atmosID_buf, 40),
          ADesc.FirstFrame);
}

//
bool
ASDCP::ATMOS::IsDolbyAtmos(const std::string& filename)
{
    // TODO
    // For now use an atmos extension
    bool result = ( 0 == (std::string("atmos").compare(Kumu::PathGetExtension(filename))) );
    return result;
}


//------------------------------------------------------------------------------------------

typedef std::list<MXF::InterchangeObject*> SubDescriptorList_t;

class ASDCP::ATMOS::MXFReader::h__Reader : public ASDCP::h__ASDCPReader
{
  MXF::PrivateDCDataDescriptor* m_EssenceDescriptor;
  MXF::DolbyAtmosSubDescriptor* m_EssenceSubDescriptor;

  KM_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

 public:
  ASDCP::DCData::DCDataDescriptor m_DDesc;
  AtmosDescriptor m_ADesc;

  h__Reader(const Dictionary& d) :
    ASDCP::h__ASDCPReader(d),  m_EssenceDescriptor(0), m_EssenceSubDescriptor(0) {}
  virtual ~h__Reader() {}
  Result_t    OpenRead(const std::string&);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_DCData_DDesc(ASDCP::DCData::DCDataDescriptor& DDesc);
  Result_t    MD_to_Atmos_ADesc(ATMOS::AtmosDescriptor& ADesc);
};

ASDCP::Result_t
ASDCP::ATMOS::MXFReader::h__Reader::MD_to_DCData_DDesc(ASDCP::DCData::DCDataDescriptor& DDesc)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);
  MXF::PrivateDCDataDescriptor* DDescObj = m_EssenceDescriptor;
  DDesc.EditRate = DDescObj->SampleRate;
  assert(DDescObj->ContainerDuration <= 0xFFFFFFFFL);
  DDesc.ContainerDuration = static_cast<ui32_t>(DDescObj->ContainerDuration);
  memcpy(DDesc.DataEssenceCoding, DDescObj->DataEssenceCoding.Value(), SMPTE_UL_LENGTH);
  return RESULT_OK;
}

ASDCP::Result_t
ASDCP::ATMOS::MXFReader::h__Reader::MD_to_Atmos_ADesc(ATMOS::AtmosDescriptor& ADesc)
{
  ASDCP_TEST_NULL(m_EssenceSubDescriptor);
  Result_t result = MD_to_DCData_DDesc(ADesc);
  if( ASDCP_SUCCESS(result) )
  {
    MXF::DolbyAtmosSubDescriptor* ADescObj = m_EssenceSubDescriptor;
    ADesc.MaxChannelCount = ADescObj->MaxChannelCount;
    ADesc.MaxObjectCount = ADescObj->MaxObjectCount;
    ::memcpy(ADesc.AtmosID, ADescObj->AtmosID.Value(), UUIDlen);
    ADesc.AtmosVersion = ADescObj->AtmosVersion;
    ADesc.FirstFrame = ADescObj->FirstFrame;
  }
  return result;
}

//
//
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename);
  m_EssenceDescriptor = 0;

  if ( KM_SUCCESS(result) )
    {
      InterchangeObject* iObj = 0;
      result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(PrivateDCDataDescriptor), &iObj);

      if ( KM_SUCCESS(result) )
	{
	  m_EssenceDescriptor = static_cast<MXF::PrivateDCDataDescriptor*>(iObj);
	}
    }

  if ( m_EssenceDescriptor == 0 )
    {
      DefaultLogSink().Error("DCDataDescriptor object not found in Atmos file.\n");
      result = RESULT_FORMAT;
    }

  if ( KM_SUCCESS(result) )
    {
      result = MD_to_DCData_DDesc(m_DDesc);
    }

  // check for sample/frame rate sanity
  if ( ASDCP_SUCCESS(result)
       && m_DDesc.EditRate != EditRate_24
       && m_DDesc.EditRate != EditRate_25
       && m_DDesc.EditRate != EditRate_30
       && m_DDesc.EditRate != EditRate_48
       && m_DDesc.EditRate != EditRate_50
       && m_DDesc.EditRate != EditRate_60
       && m_DDesc.EditRate != EditRate_96
       && m_DDesc.EditRate != EditRate_100
       && m_DDesc.EditRate != EditRate_120
       && m_DDesc.EditRate != EditRate_192
       && m_DDesc.EditRate != EditRate_200
       && m_DDesc.EditRate != EditRate_240 )
  {
    DefaultLogSink().Error("DC Data file EditRate is not a supported value: %d/%d\n", // lu
                           m_DDesc.EditRate.Numerator, m_DDesc.EditRate.Denominator);

    return RESULT_FORMAT;
  }

  if( ASDCP_SUCCESS(result) )
    {
      
      if (NULL == m_EssenceSubDescriptor)
	{
	  InterchangeObject* iObj = NULL;
	  result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(DolbyAtmosSubDescriptor), &iObj);
	  m_EssenceSubDescriptor = static_cast<MXF::DolbyAtmosSubDescriptor*>(iObj);
	  
	  if ( iObj == 0 )
	    {
	      DefaultLogSink().Error("DolbyAtmosSubDescriptor object not found.\n");
	      return RESULT_FORMAT;
	    }
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  result = MD_to_Atmos_ADesc(m_ADesc);
	}
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_PrivateDCDataEssence), Ctx, HMAC);
}


//------------------------------------------------------------------------------------------

ASDCP::ATMOS::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(AtmosSMPTEDict());
}


ASDCP::ATMOS::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::ATMOS::MXFReader::OP1aHeader()
{
  if ( m_Reader.empty() )
  {
    assert(g_OP1aHeader);
    return *g_OP1aHeader;
  }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::ATMOS::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
  {
    assert(g_OPAtomIndexFooter);
    return *g_OPAtomIndexFooter;
  }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::ATMOS::MXFReader::RIP()
{
  if ( m_Reader.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::ReadFrame(ui32_t FrameNum, DCData::FrameBuffer& FrameBuf,
                                    AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::ATMOS::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::FillAtmosDescriptor(AtmosDescriptor& ADesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
  {
    ADesc = m_Reader->m_ADesc;
    return RESULT_OK;
  }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
  {
    Info = m_Reader->m_Info;
    return RESULT_OK;
  }

  return RESULT_INIT;
}

//
void
ASDCP::ATMOS::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}

//
void
ASDCP::ATMOS::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------

//
class ASDCP::ATMOS::MXFWriter::h__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP::DCData::DCDataDescriptor m_DDesc;
  byte_t  m_EssenceUL[SMPTE_UL_LENGTH];
  MXF::DolbyAtmosSubDescriptor* m_EssenceSubDescriptor;

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

 public:
  AtmosDescriptor m_ADesc;

  h__Writer(const Dictionary& d) : ASDCP::h__ASDCPWriter(d),
				   m_EssenceSubDescriptor(0), m_ADesc() {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ui32_t HeaderSize, const AtmosDescriptor& ADesc);
  Result_t SetSourceStream(const DCData::DCDataDescriptor&, const byte_t*, const std::string&, const std::string&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t DCData_DDesc_to_MD(ASDCP::DCData::DCDataDescriptor& DDesc);
  Result_t Atmos_ADesc_to_MD(const AtmosDescriptor& ADesc);
};

//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::DCData_DDesc_to_MD(ASDCP::DCData::DCDataDescriptor& DDesc)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);
  MXF::PrivateDCDataDescriptor* DDescObj = static_cast<MXF::PrivateDCDataDescriptor *>(m_EssenceDescriptor);

  DDescObj->SampleRate = DDesc.EditRate;
  DDescObj->ContainerDuration = DDesc.ContainerDuration;
  DDescObj->DataEssenceCoding.Set(DDesc.DataEssenceCoding);  
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::Atmos_ADesc_to_MD(const AtmosDescriptor& ADesc)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);
  ASDCP_TEST_NULL(m_EssenceSubDescriptor);
  MXF::DolbyAtmosSubDescriptor * ADescObj = m_EssenceSubDescriptor;
  ADescObj->MaxChannelCount = ADesc.MaxChannelCount;
  ADescObj->MaxObjectCount = ADesc.MaxObjectCount;
  ADescObj->AtmosID.Set(ADesc.AtmosID);
  ADescObj->AtmosVersion = ADesc.AtmosVersion;
  ADescObj->FirstFrame = ADesc.FirstFrame;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ui32_t HeaderSize, const AtmosDescriptor& ADesc)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new MXF::PrivateDCDataDescriptor(m_Dict);
      m_EssenceSubDescriptor = new DolbyAtmosSubDescriptor(m_Dict);
      SubDescriptorList_t subDescriptors;
      subDescriptors.push_back(m_EssenceSubDescriptor);

      SubDescriptorList_t::const_iterator sDObj;
      SubDescriptorList_t::const_iterator lastDescriptor = subDescriptors.end();
      for (sDObj = subDescriptors.begin(); sDObj != lastDescriptor; ++sDObj)
      {
          m_EssenceSubDescriptorList.push_back(*sDObj);
          GenRandomValue((*sDObj)->InstanceUID);
          m_EssenceDescriptor->SubDescriptors.push_back((*sDObj)->InstanceUID);
      }
      result = m_State.Goto_INIT();
    }

  if ( ASDCP_FAILURE(result) )
    delete m_EssenceSubDescriptor;

  if ( ASDCP_SUCCESS(result) )
  {
      m_ADesc = ADesc;
      memcpy(m_ADesc.DataEssenceCoding, ATMOS_ESSENCE_CODING, SMPTE_UL_LENGTH);
      result = Atmos_ADesc_to_MD(m_ADesc);
  }

  return result;
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::SetSourceStream(ASDCP::DCData::DCDataDescriptor const& DDesc,
						    const byte_t * essenceCoding,
						    const std::string& packageLabel,
						    const std::string& defLabel)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( DDesc.EditRate != EditRate_24
       && DDesc.EditRate != EditRate_25
       && DDesc.EditRate != EditRate_30
       && DDesc.EditRate != EditRate_48
       && DDesc.EditRate != EditRate_50
       && DDesc.EditRate != EditRate_60
       && DDesc.EditRate != EditRate_96
       && DDesc.EditRate != EditRate_100
       && DDesc.EditRate != EditRate_120
       && DDesc.EditRate != EditRate_192
       && DDesc.EditRate != EditRate_200
       && DDesc.EditRate != EditRate_240 )
  {
    DefaultLogSink().Error("DCDataDescriptor.EditRate is not a supported value: %d/%d\n",
                           DDesc.EditRate.Numerator, DDesc.EditRate.Denominator);
    return RESULT_RAW_FORMAT;
  }

  assert(m_Dict);
  m_DDesc = DDesc;
  if (NULL != essenceCoding)
      memcpy(m_DDesc.DataEssenceCoding, essenceCoding, SMPTE_UL_LENGTH);
  Result_t result = DCData_DDesc_to_MD(m_DDesc);

  if ( ASDCP_SUCCESS(result) )
  {
    memcpy(m_EssenceUL, m_Dict->ul(MDD_PrivateDCDataEssence), SMPTE_UL_LENGTH);
    m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
    result = m_State.Goto_READY();
  }

  if ( ASDCP_SUCCESS(result) )
  {
    ui32_t TCFrameRate = m_DDesc.EditRate.Numerator;

    result = WriteASDCPHeader(packageLabel, UL(m_Dict->ul(MDD_PrivateDCDataWrappingFrame)),
			      defLabel, UL(m_EssenceUL), UL(m_Dict->ul(MDD_DataDataDef)),
			      m_DDesc.EditRate, TCFrameRate);
  }

  return result;
}

//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf,
					       ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through

  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
  {
    IndexTableSegment::IndexEntry Entry;
    Entry.StreamOffset = StreamOffset;
    m_FooterPart.PushIndexEntry(Entry);
    m_FramesWritten++;
  }
  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}



//------------------------------------------------------------------------------------------

ASDCP::ATMOS::MXFWriter::MXFWriter()
{
}

ASDCP::ATMOS::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::ATMOS::MXFWriter::OP1aHeader()
{
  if ( m_Writer.empty() )
  {
    assert(g_OP1aHeader);
    return *g_OP1aHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::ATMOS::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
  {
    assert(g_OPAtomIndexFooter);
    return *g_OPAtomIndexFooter;
  }

  return m_Writer->m_FooterPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::ATMOS::MXFWriter::RIP()
{
  if ( m_Writer.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				       const AtmosDescriptor& ADesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType != LS_MXF_SMPTE )
  {
    DefaultLogSink().Error("Atmos support requires LS_MXF_SMPTE\n");
    return RESULT_FORMAT;
  }

  m_Writer = new h__Writer(AtmosSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, HeaderSize, ADesc);

  if ( ASDCP_SUCCESS(result) )
      result = m_Writer->SetSourceStream(ADesc, ATMOS_ESSENCE_CODING, ATMOS_PACKAGE_LABEL,
                                         ATMOS_DEF_LABEL);
  
  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::WriteFrame(const DCData::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::ATMOS::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_DCP_ATMOS.cpp
//


