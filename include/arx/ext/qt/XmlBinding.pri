# This file is part of ArXLib, a C++ ArX Primitives Library.
#
# Copyright (C) 2008-2011 Alexander Fokin <apfokin@gmail.com>
#
# ArXLib is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# ArXLib is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License 
# for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with ArXLib. If not, see <http://www.gnu.org/licenses/>. 
# 
# $Id: FormPrinter.pri 114 2010-08-24 14:33:46Z ru.elric $
include(../../Arx.pri)

QT += xml

HEADERS += \
  $$PWD/XmlBinding.h \
  $$PWD/XmlQDomNodeInspector.h \
  $$PWD/XmlQDomNodeWalker.h \
  $$PWD/XmlQStringProcessor.h \

DEFINES     += ARX_XML_BINDING_PRI_INCLUDED
