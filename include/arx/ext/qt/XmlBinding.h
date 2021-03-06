/* This file is part of ArXLib, a C++ ArX Primitives Library.
 *
 * Copyright (C) 2008-2011 Alexander Fokin <apfokin@gmail.com>
 *
 * ArXLib is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ArXLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ArXLib. If not, see <http://www.gnu.org/licenses/>. 
 * 
 * $Id$ */
#ifndef ARX_EXT_QT_XML_BINDING_H
#define ARX_EXT_QT_XML_BINDING_H

#include <arx/config.h>
#include <cassert>
#include <limits>
#include <boost/range/value_type.hpp>
#include <boost/range/end.hpp>
#include <QString>
#include <QDomNode>
#include <arx/range/Insert.h>
#include <arx/ext/qt/Range.h>
#include <arx/xml/Binding.h>
#include <arx/Foreach.h>
#include "XmlQDomNodeInspector.h"
#include "XmlQDomNodeWalker.h"
#include "XmlQStringProcessor.h"

#ifndef ARX_XML_BINDING_PRI_INCLUDED
#  error Include XmlBinding.pri into your qmake project file before including this file
#endif

template<class T> class QList;
template<class T> class QVector;
template<class T> class QLinkedList;

namespace arx { namespace xml {
  namespace qt_xml_binding_detail {
    /**
     * Serialization function for QStrings.
     */
    inline void serialize(const QString &source, QDomNode *target) {
      assert((target->isAttr() && target->toAttr().value().isNull()) || (target->isElement() && target->childNodes().size() == 0));

      if(target->isAttr())
        target->toAttr().setValue(source);
      else
        target->appendChild(target->ownerDocument().createTextNode(source));
    }


    /**
     * Deserialization function for QStrings.
     */
    inline QString deserialize(const QDomNode &source) {
      assert(source.isAttr() || source.isElement());

      if(source.isAttr())
        return source.toAttr().value();
      else
        return source.toElement().text();
    }


    /**
     * Tag for element_name parameter.
     */
    struct element_name_tag {};

    ARX_DEFINE_PROPERTY_KEY(element_name_tag, element_name);


    /**
     * Serializer functor that implements serialization of basic C++ types and 
     * QStrings.
     */
    struct Serializer {
      template<class MessageTranslator, class Params>
      void operator()(const QString &source, MessageTranslator &, const Params &, QDomNode *target) const {
        serialize(source, target);
      }

#define ARX_SERIALIZATION_FUNC(TYPE, EXPR)                                      \
      template<class MessageTranslator, class Params>                           \
      void operator()(TYPE x, MessageTranslator &, const Params &, QDomNode *target) const { \
        serialize(EXPR, target);                                                \
      }

      ARX_SERIALIZATION_FUNC(bool,                         QString::fromLatin1(x ? "1" : "0"));
      ARX_SERIALIZATION_FUNC(char,                         QString::number(static_cast<int>(x)));
      ARX_SERIALIZATION_FUNC(unsigned char,                QString::number(static_cast<unsigned int>(x)));
      ARX_SERIALIZATION_FUNC(short,                        QString::number(static_cast<int>(x)));
      ARX_SERIALIZATION_FUNC(unsigned short,               QString::number(static_cast<unsigned int>(x)));
      ARX_SERIALIZATION_FUNC(int,                          QString::number(x));
      ARX_SERIALIZATION_FUNC(unsigned int,                 QString::number(x));
      ARX_SERIALIZATION_FUNC(long,                         QString::number(x));
      ARX_SERIALIZATION_FUNC(unsigned long,                QString::number(x));
      ARX_SERIALIZATION_FUNC(long long,                    QString::number(x));
      ARX_SERIALIZATION_FUNC(unsigned long long,           QString::number(x));
      ARX_SERIALIZATION_FUNC(float,                        QString::number(static_cast<float>(x), 'g', 9));
      ARX_SERIALIZATION_FUNC(double,                       QString::number(x, 'g', 18));
#undef ARX_SERIALIZATION_FUNC
    };
    
    struct CollectionSerializer {
      template<class Collection, class MessageTranslator, class Params>
      void operator()(const Collection &source, MessageTranslator &translator, const Params &params, QDomNode *target) const {
        typedef typename boost::range_value<Collection>::type value_type;

        QString elementName = QString(params.template get<element_name_tag>(static_cast<const char *>("elem"))); /* TODO: handle this in a normal way. */
        foreach(const value_type &value, source) {
          QDomNode element = target->appendChild(target->ownerDocument().createElement(elementName));
          arx::xml::serialize(value, translator, params, &element);
        }
      }
    };


    /**
     * Deserialization functor that implements deserialization of basic C++
     * types and QStrings.
     */
    struct Deserializer {
      template<class MessageTranslator, class Params>
      void operator()(QDomNode &source, MessageTranslator &, const Params &, QString *target) const {
        *target = deserialize(source);
      }

#define ARX_DESERIALIZATION_FUNC(TYPE, INTERMEDIATE_EXPR, CHECK_EXPR)           \
      template<class MessageTranslator, class Params>                           \
      void operator()(QDomNode &source, MessageTranslator &handler, const Params &, TYPE *target) const { \
        QString s = deserialize(source);                                        \
        bool ok = true;                                                         \
        decltype(INTERMEDIATE_EXPR) x = (INTERMEDIATE_EXPR);                    \
        if(ok)                                                                  \
          ok = (CHECK_EXPR);                                                    \
        if(ok)                                                                  \
          *target = static_cast<TYPE>(x);                                       \
        else                                                                    \
          handler(ERROR, create_invalid_value_for_type<TYPE>(s), source);       \
      }

      ARX_DESERIALIZATION_FUNC(bool,                       
        (s.size() == 1) ? (s[0] != QChar::fromLatin1('0')) : (ok = false), 
        s[0] == QChar::fromLatin1('0') || s[0] == QChar::fromLatin1('1')
      );

#define ARX_INT_DESERIALIZATION_FUNC(TYPE, INTERMEDIATE_EXPR)                   \
      ARX_DESERIALIZATION_FUNC(TYPE, INTERMEDIATE_EXPR, (x >= std::numeric_limits<TYPE>::min() && x <= std::numeric_limits<TYPE>::max()))

      ARX_INT_DESERIALIZATION_FUNC(char,                   s.toInt(&ok));
      ARX_INT_DESERIALIZATION_FUNC(unsigned char,          s.toInt(&ok));
      ARX_INT_DESERIALIZATION_FUNC(short,                  s.toInt(&ok));
      ARX_INT_DESERIALIZATION_FUNC(unsigned short,         s.toInt(&ok));
#undef ARX_INT_DESERIALIZATION_FUNC

      ARX_DESERIALIZATION_FUNC(int,                        s.toInt(&ok),        true);
      ARX_DESERIALIZATION_FUNC(unsigned int,               s.toUInt(&ok),       true);
      ARX_DESERIALIZATION_FUNC(long,                       s.toLong(&ok),       true);
      ARX_DESERIALIZATION_FUNC(unsigned long,              s.toULong(&ok),      true);
      ARX_DESERIALIZATION_FUNC(long long,                  s.toLongLong(&ok),   true);
      ARX_DESERIALIZATION_FUNC(unsigned long long,         s.toULongLong(&ok),  true);
      ARX_DESERIALIZATION_FUNC(float,                      s.toFloat(&ok),      true);
      ARX_DESERIALIZATION_FUNC(double,                     s.toDouble(&ok),     true);
#undef ARX_DESERIALIZATION_FUNC
    };

    struct CollectionDeserializer {
      template<class Collection, class MessageTranslator, class Params>
      void operator()(QDomNode &source, MessageTranslator &translator, const Params &params, Collection *target) const {
        typedef typename boost::range_value<Collection>::type value_type;

        QString elementName = QString(params.template get<element_name_tag>(static_cast<const char *>("elem")));
        for(QDomNode child = source.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
          if(child.nodeName() != elementName) {
            translator(ERROR, create_invalid_desired_name(child.nodeName(), elementName), child);
          } else {
            value_type value;
            if(arx::xml::deserialize(child, translator, params, &value))
              arx::insert(*target, boost::end(*target), std::move(value));
          }
        }
      }

    };

  } // namespace qt_xml_binding_detail


#define ARX_DEFINE_STANDARD_BINDING(TYPE)                                       \
  ARX_XML_DEFINE_BINDING(                                                       \
    TYPE,                                                                       \
    ARX_XML_FUNCTIONAL(                                                         \
      self,                                                                     \
      qt_xml_binding_detail::Serializer(),                                      \
      qt_xml_binding_detail::Deserializer()                                     \
    )                                                                           \
  ) 

  ARX_DEFINE_STANDARD_BINDING(QString);
  ARX_DEFINE_STANDARD_BINDING(bool);
  ARX_DEFINE_STANDARD_BINDING(char);
  ARX_DEFINE_STANDARD_BINDING(unsigned char);
  ARX_DEFINE_STANDARD_BINDING(short);
  ARX_DEFINE_STANDARD_BINDING(unsigned short);
  ARX_DEFINE_STANDARD_BINDING(int);
  ARX_DEFINE_STANDARD_BINDING(unsigned int);
  ARX_DEFINE_STANDARD_BINDING(long);
  ARX_DEFINE_STANDARD_BINDING(unsigned long);
  ARX_DEFINE_STANDARD_BINDING(long long);
  ARX_DEFINE_STANDARD_BINDING(unsigned long long);
  ARX_DEFINE_STANDARD_BINDING(float);
  ARX_DEFINE_STANDARD_BINDING(double);
#undef ARX_DEFINE_STANDARD_BINDING

#define ARX_DEFINE_STANDARD_COLLECTION_NAMED_TPL_BINDING(NAME, TYPE, TYPE_TPL_SEQ, TYPE_SPEC_SEQ) \
  ARX_XML_DEFINE_NAMED_TPL_BINDING(                                             \
    NAME,                                                                       \
    TYPE,                                                                       \
    TYPE_TPL_SEQ,                                                               \
    TYPE_SPEC_SEQ,                                                              \
    ARX_XML_FUNCTIONAL(                                                         \
      self,                                                                     \
      qt_xml_binding_detail::CollectionSerializer(),                            \
      qt_xml_binding_detail::CollectionDeserializer()                           \
    )                                                                           \
  ) 

  ARX_DEFINE_STANDARD_COLLECTION_NAMED_TPL_BINDING(QList_xml_binding,       QList,       (class)(T), (T));
  ARX_DEFINE_STANDARD_COLLECTION_NAMED_TPL_BINDING(QVector_xml_binding,     QVector,     (class)(T), (T));
  ARX_DEFINE_STANDARD_COLLECTION_NAMED_TPL_BINDING(QLinkedList_xml_binding, QLinkedList, (class)(T), (T));
#undef ARX_DEFINE_STANDARD_NAMED_TPL_BINDING

  using qt_xml_binding_detail::element_name;

}} // namespace arx::xml

#endif // ARX_EXT_QT_XML_BINDING_H
