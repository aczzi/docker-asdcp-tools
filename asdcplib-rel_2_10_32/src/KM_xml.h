/*
Copyright (c) 2005-2015, John Hurst
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
/*! \file    KM_xml.h
    \version $Id$
    \brief   XML writer
*/


#ifndef _KM_XML_H_
#define _KM_XML_H_

#include <KM_util.h>
#include <list>
#include <set>
#include <string>

namespace Kumu
{
  class XMLElement;

  //
  struct NVPair
  {
    std::string name;
    std::string value;
  };

  //
  typedef std::list<NVPair> AttributeList;
  typedef AttributeList::const_iterator Attr_i;
  typedef std::list<XMLElement*> ElementList;
  typedef ElementList::const_iterator Elem_i;

  bool GetXMLDocType(const ByteString& buf, std::string& ns_prefix, std::string& type_name,
		     std::string& namespace_name, AttributeList& doc_attr_list);

  bool GetXMLDocType(const std::string& buf, std::string& ns_prefix, std::string& type_name,
		     std::string& namespace_name, AttributeList& doc_attr_list);

  bool GetXMLDocType(const byte_t* buf, ui32_t buf_len, std::string& ns_prefix, std::string& type_name,
		     std::string& namespace_name, AttributeList& doc_attr_list);

  //
  class XMLNamespace
  {
    std::string   m_Prefix;
    std::string   m_Name;

    KM_NO_COPY_CONSTRUCT(XMLNamespace);
    XMLNamespace();

    public:
  XMLNamespace(const char* prefix, const char* name) : m_Prefix(prefix), m_Name(name) {}
    ~XMLNamespace() {}

    inline const std::string& Prefix() const { return m_Prefix; }
    inline const std::string& Name() const { return m_Name; }
  };

  //
  class XMLElement
    {
      KM_NO_COPY_CONSTRUCT(XMLElement);
      XMLElement();

    protected:
      AttributeList       m_AttrList;
      ElementList         m_ChildList;
      const XMLNamespace* m_Namespace;
      void*               m_NamespaceOwner;

      std::string   m_Name;
      std::string   m_Body;

    public:
      XMLElement(const char* name);
      ~XMLElement();

      inline const XMLNamespace* Namespace() const { return m_Namespace; }
      inline void                SetNamespace(const XMLNamespace* ns) { assert(ns); m_Namespace = ns; }

      bool        ParseString(const char* document, ui32_t doc_len);
      bool        ParseString(const ByteString& document);
      bool        ParseString(const std::string& document);

      bool        ParseFirstFromString(const char* document, ui32_t doc_len);
      bool        ParseFirstFromString(const ByteString& document);
      bool        ParseFirstFromString(const std::string& document);

      // building
      void        SetName(const char* name);
      void        SetBody(const std::string& value);
      void        AppendBody(const std::string& value);
      void        SetAttr(const char* name, const char* value);
      void        SetAttr(const char* name, const std::string& value) { SetAttr(name, value.c_str()); }
      XMLElement* AddChild(XMLElement* element);
      XMLElement* AddChild(const char* name);
      XMLElement* AddChildWithContent(const char* name, const char* value);
      XMLElement* AddChildWithContent(const char* name, const std::string& value);
      XMLElement* AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value);
      void        AddComment(const char* value);
      void        Render(std::string& str) const { Render(str, true); }
      void        Render(std::string&, const bool& pretty) const;
      void        RenderElement(std::string& outbuf, const ui32_t& depth, const bool& pretty) const;

      // querying
      inline const std::string&   GetBody() const { return m_Body; }
      inline const ElementList&   GetChildren() const { return m_ChildList; }
      inline const std::string&   GetName() const { return m_Name; }
      inline const AttributeList& GetAttributes() const { return m_AttrList; }
      const char*        GetAttrWithName(const char* name) const;
      XMLElement*        GetChildWithName(const char* name) const;
      const ElementList& GetChildrenWithName(const char* name, ElementList& outList) const;
      bool               HasName(const char* name) const;

      // altering
      void        DeleteAttributes();
      void        DeleteAttrWithName(const char* name);
      void        DeleteChildren();
      void        DeleteChild(const XMLElement* element);
      void        ForgetChild(const XMLElement* element);
    };

  //
  template <class VisitorType>
    bool
    apply_visitor(const XMLElement& element, VisitorType& visitor)
    {
      const ElementList& l = element.GetChildren();
      ElementList::const_iterator i;
      
      for ( i = l.begin(); i != l.end(); ++i )
	{
	  if ( ! visitor.Element(**i) )
	    {
	      return false;
	    }

	  if ( ! apply_visitor(**i, visitor) )
	    {
	      return false;
	    }
	}

      return true;
    }

  //
  class AttributeVisitor
  {
    std::string attr_name;

  public:
  AttributeVisitor(const std::string& n) : attr_name(n) {}
    std::set<std::string> value_list;

    bool Element(const XMLElement& e)
    {
      const AttributeList& l = e.GetAttributes();
      AttributeList::const_iterator i;
 
      for ( i = l.begin(); i != l.end(); ++i )
	{
	  if ( i->name == attr_name )
	    {
	      value_list.insert(i->value);
	    }
	}

      return true;
    }
  };

  //
  class ElementVisitor
  {
    std::string element_name;

  public:
  ElementVisitor(const std::string& n) : element_name(n) {}
    std::set<std::string> value_list;

    bool Element(const XMLElement& e)
    {
      if ( e.GetBody() == element_name )
	{
	  value_list.insert(e.GetBody());
	}

      return true;
    }
  };

} // namespace Kumu

#endif // _KM_XML_H_

//
// end KM_xml.h
//
