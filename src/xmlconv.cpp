/*
 * xmlconv.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2002-2003 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 * for use under the terms of the GNU General Public License,
 * incorporated herein by reference.
 *
 */

#define __STDC_LIMIT_MACROS

#include "pch.h"

#include "xmlconv.h"

static bool ok_in_hex(QChar c)
{
	return c.isDigit() || (c >= 'a' && c <= 'f') || (c >= 'A' && c < 'F');
}

DomConvenience::DomConvenience(QDomDocument& doc)
: m_doc(doc)
{
}

bool DomConvenience::elementToVariant(const QDomElement& e, QVariant& v)
{
	bool ok = false;

	if (e.tagName() == "string")
	{
		if (e.hasAttribute("value"))
		{
			v = QVariant(e.attribute("value"));
			ok = true;
		}
		else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "int")
	{
		int base = getBase(e);

		if (e.hasAttribute("value"))
			v = QVariant(e.attribute("value").toInt(&ok, base));
		else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "uint")
	{
		int base = getBase(e);

		if (e.hasAttribute("value"))
			v = QVariant(e.attribute("value").toUInt(&ok, base));
		else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "double")
	{
		if (e.hasAttribute("value"))
			v = QVariant(e.attribute("value").toDouble(&ok));
		else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "bool")
	{
		if (e.hasAttribute("value"))
		{
			QString val = e.attribute("value");
			v = QVariant(getBoolFromString(val, ok), 0);

			if (!ok)
				qWarning("%s element with bogus value '%s'!", (const char*)e.tagName(), (const char*)val);
		}
		else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "color")
	{
		QColor color = getColor(e);
		ok = color.isValid();
		v = color;

		if (!ok)
			qWarning("%s element without valid value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "pen")
	{
		int base = getBase(e);
		uint w = 0;
		Qt::PenStyle s = Qt::SolidLine;
		Qt::PenCapStyle c = Qt::SquareCap;
		Qt::PenJoinStyle j = Qt::BevelJoin;
		QColor color = getColor(e);

		if (e.hasAttribute("style"))
			s = (Qt::PenStyle)e.attribute("style").toInt(0, base);
		if (e.hasAttribute("cap"))
			c = (Qt::PenCapStyle)e.attribute("cap").toInt(0, base);
		if (e.hasAttribute("join"))
			j = (Qt::PenJoinStyle)e.attribute("join").toInt(0, base);

		ok = color.isValid();

		v = QPen(color, w, s, c, j);

		if (!ok)
			qWarning("%s element without valid value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "brush")
	{
		int base = getBase(e);
		QColor color = getColor(e);
		Qt::BrushStyle s = Qt::SolidPattern;
		if (e.hasAttribute("style"))
			s = (Qt::BrushStyle)e.attribute("style").toInt(0, base);

		ok = color.isValid();

		v = QBrush(color, s);

		if (!ok)
			qWarning("%s element without valid value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "point")
	{
		int base = getBase(e);
		bool coordOk;

		int x = 0, y = 0;
		if (e.hasAttribute("x"))
			x = e.attribute("x").toInt(&coordOk, base);

		if (e.hasAttribute("y"))
			y = e.attribute("y").toInt(&coordOk, base);

		v = QVariant(QPoint(x, y));
		ok = true;
	}
	else if (e.tagName() == "rect")
	{
		int base = getBase(e);
		bool coordOk;

		int x = 0, y = 0, width = 0, height = 0;
		if (e.hasAttribute("x"))
			x = e.attribute("x").toInt(&coordOk, base);

		if (e.hasAttribute("y"))
			y = e.attribute("y").toInt(&coordOk, base);

		if (e.hasAttribute("width"))
			width = e.attribute("width").toInt(&coordOk, base);

		if (e.hasAttribute("height"))
			height = e.attribute("height").toInt(&coordOk, base);

		v = QVariant(QRect(x, y, width, height));
		ok = true;
	}
	else if (e.tagName() == "size")
	{
		int base = getBase(e);
		bool coordOk;

		int width = 0, height = 0;
		if (e.hasAttribute("width"))
			width = e.attribute("width").toInt(&coordOk, base);

		if (e.hasAttribute("height"))
			height = e.attribute("height").toInt(&coordOk, base);

		v = QVariant(QSize(width, height));
		ok = true;
	}
	else if (e.tagName() == "key")
	{
		QKeySequence key;
		if (e.hasAttribute("sequence"))
		{
			key = (QKeySequence)e.attribute("sequence");

			// fix the key code (deal with Qt brain death)
			v = key & ~Qt::UNICODE_ACCEL;
			ok = true;
		}
	}
	else if (e.tagName() == "font")
	{
		QFont f;
		bool boolOk;

		if (e.hasAttribute("family"))
			f.setFamily(e.attribute("family"));
		if (e.hasAttribute("pointsize"))
			f.setPointSize(e.attribute("pointsize").toInt());
		if (e.hasAttribute("bold"))
			f.setBold(getBoolFromString(e.attribute("bold"), boolOk));
		if (e.hasAttribute("italic"))
			f.setItalic(getBoolFromString(e.attribute("italic"), boolOk));
		if (e.hasAttribute("strikeout"))
			f.setStrikeOut(getBoolFromString(e.attribute("strikeout"), boolOk));

		v = QVariant(f);
		ok = true;
	}
	else if (e.tagName() == "sizepolicy")
	{
		QSizePolicy sp;

		if (e.hasAttribute("hsizetype"))
			sp.setHorData((QSizePolicy::SizeType)e.attribute("hsizetype").toInt());
		if (e.hasAttribute("vsizetype"))
			sp.setVerData((QSizePolicy::SizeType)e.attribute("vsizetype").toInt());
		if (e.hasAttribute("horstretch"))
			sp.setHorStretch(e.attribute("horstretch").toInt());
		if (e.hasAttribute("verstretch"))
			sp.setHorStretch(e.attribute("verstretch").toInt());
		v = QVariant(sp);
		ok = true;
	}
	else if (e.tagName() == "cursor")
	{
		if (e.hasAttribute("shape")) {
			v = QVariant(QCursor((Qt::CursorShape)e.attribute("shape").toInt(&ok, 10)));
		} else
			qWarning("%s element without value!", (const char*)e.tagName());
	}
	else if (e.tagName() == "stringlist")
	{
		QDomNodeList stringNodeList = e.elementsByTagName("string");
		QStringList stringList;
		QDomElement stringElement;

		for (uint i = 0; i < stringNodeList.length(); i++)
		{
			stringElement = stringNodeList.item(i).toElement();
			if (!stringElement.hasAttribute("value"))
			{
				qWarning("%s element in %s without value! Ignoring!",
						 (const char*)stringElement.tagName(),
						 (const char*)e.tagName());
				continue;
			}

			stringList.append(e.attribute("value"));
		}

		v = stringList;

		ok = true;
	}
	else if (e.tagName() == "uint64")
	{
		QString value = e.attribute("value");

		// borrowed more or less from Qt 3.2 (since we have to support older)
		uint64_t val = 0;
		const QChar* p = value.unicode();
		int l = value.length();
		const uint64_t max_mult = 0x0fffffffffffffffull/* UINT64_MAX / 16 */;

		if (!p)
		{
			qWarning("Invalid value for tag: %s", (const char*)e.tagName());
			return false;
		}

		while ( l && p->isSpace() )                 // skip leading space
			l--,p++;
		if ( !l )
			return false;
		if ( *p == '+' )
			l--,p++;

		if ( !l || !ok_in_hex(*p) )
			return false;
		while ( l && ok_in_hex(*p) )
		{
			l--;
			uint dv;
			if ( p->isDigit() )
				dv = p->digitValue();
			else
			{
				if ( *p >= 'a' && *p <= 'f' )
					dv = p->toAscii() - 'a' + 10;
				else
					dv = p->toAscii() - 'A' + 10;
			}
			if ( val > max_mult || (val == max_mult && dv > 0xf /*UINT64_MAX % 16*/) )
				return false;
			val = 16 * val + dv;
			p++;
		}

		QByteArray ba;
		ba.duplicate((const char*)&val, sizeof(uint64_t));

		v = ba;
		ok = true;
	}
	else if (e.tagName() == "data")
	{
		v = getByteArrayFromString(e.attribute("value"));
		ok = true;
	}
	else if (e.tagName() == "list")
	{
		qWarning("Unimplemented tag: %s", (const char*)e.tagName());
	}
	else if (e.tagName() == "map")
	{
		qWarning("Unimplemented tag: %s", (const char*)e.tagName());
	}
	else
	{
		qWarning("Unknown tag: %s", (const char*)e.tagName());
	}

	return ok;
}

bool DomConvenience::variantToElement(const QVariant& v, QDomElement& e)
{
	bool ok = true;

	clearAttributes(e);

	switch (v.type())
	{
		case QVariant::String:
			e.setTagName("string");
			e.setAttribute("value", v.toString());
			break;
#if 0
		case QVariant::CString:
			e.setTagName("string");
			e.setAttribute("value", v.toString());
			break;
#endif
		case QVariant::Int:
			e.setTagName("int");
			e.setAttribute("value", v.toInt());
			break;
		case QVariant::UInt:
			e.setTagName("uint");
			e.setAttribute("value", v.toUInt());
			break;
		case QVariant::Double:
			e.setTagName("double");
			e.setAttribute("value", v.toDouble());
			break;
		case QVariant::Bool:
			e.setTagName("bool");
			e.setAttribute("value", boolString(v.toBool()));
			break;
		case QVariant::Color:
		{
			e.setTagName("color");
			QColor color = v.value<QColor>();
			e.setAttribute("red", color.red());
			e.setAttribute("green", color.green());
			e.setAttribute("blue", color.blue());
		}
			break;
		case QVariant::Pen:
		{
			e.setTagName("pen");
			QPen pen = v.value<QPen>();
			e.setAttribute("red", pen.color().red());
			e.setAttribute("green", pen.color().green());
			e.setAttribute("blue", pen.color().blue());
			e.setAttribute("style", pen.style());
			e.setAttribute("cap", pen.capStyle());
			e.setAttribute("join", pen.joinStyle());
		}
			break;
		case QVariant::Brush:
		{
			e.setTagName("brush");
			QBrush brush = v.value<QBrush>();
			e.setAttribute("red", brush.color().red());
			e.setAttribute("green", brush.color().green());
			e.setAttribute("blue", brush.color().blue());
			e.setAttribute("style", brush.style());
		}
			break;
		case QVariant::Point:
		{
			e.setTagName("point");
			QPoint point = v.toPoint();
			e.setAttribute("x", point.x());
			e.setAttribute("y", point.y());
		}
			break;
		case QVariant::Rect:
		{
			e.setTagName("rect");
			QRect rect = v.toRect();
			e.setAttribute("x", rect.x());
			e.setAttribute("y", rect.y());
			e.setAttribute("width", rect.width());
			e.setAttribute("height", rect.height());
		}
			break;
		case QVariant::Size:
		{
			e.setTagName("size");
			QSize qsize = v.toSize();
			e.setAttribute("width", qsize.width());
			e.setAttribute("height", qsize.height());
		}
			break;
		case QVariant::Font:
		{
			e.setTagName("font");
			QFont f(v.value<QFont>());
			e.setAttribute("family", f.family());
			e.setAttribute("pointsize", f.pointSize());
			e.setAttribute("bold", boolString(f.bold()));
			e.setAttribute("italic", boolString(f.italic()));
			e.setAttribute("underline", boolString(f.underline()));
			e.setAttribute("strikeout", boolString(f.strikeOut()));
		}
			break;
		case QVariant::SizePolicy:
		{
			e.setTagName("sizepolicy");
			QSizePolicy sp(v.value<QSizePolicy>());
			e.setAttribute("hsizetype", sp.horData());
			e.setAttribute("vsizetype", sp.verData());
			e.setAttribute("horstretch", sp.horStretch());
			e.setAttribute("verstretch", sp.verStretch());
		}
			break;
		case QVariant::Cursor:
			e.setTagName("cursor");
			e.setAttribute("shape", v.value<QCursor>().shape());
			break;

		case QVariant::StringList:
		{
			e.setTagName("stringlist");
			int32_t j;

			QDomNode n;
			QDomNodeList stringNodeList = e.elementsByTagName("string");
			QDomElement stringElem;
			QStringList stringList = v.toStringList();
			QStringList::Iterator it = stringList.begin();

			for (j = 0;
				 ((j < (int32_t)stringNodeList.length()) && (it != stringList.end()));
				 j++)
			{
				// get the current string element
				stringElem = stringNodeList.item(j).toElement();

				// set it to the current string
				variantToElement(QVariant(*it), stringElem);

				// iterate to the next string
				++it;
			}

			// more nodes in previous stringlist then current, remove excess nodes
			if (stringNodeList.count() > stringList.count())
			{
				while (j < (int32_t)stringNodeList.count())
					e.removeChild(stringNodeList.item(j).toElement());
			}
			else if (j < (int32_t)stringList.count())
			{
				while (it != stringList.end())
				{
					// create a new element
					stringElem = m_doc.createElement("string");

					// set it to the currentstring
					variantToElement(QVariant(*it), stringElem);

					// append it to the current element
					e.appendChild(stringElem);

					// iterate to the next string
					++it;
				}
			}
		}
			break;

		case QVariant::KeySequence:
			e.setTagName("key");
			e.setAttribute("sequence", (QString)v.value<QKeySequence>().toString());
			break;

		case QVariant::ByteArray: // this is only for [u]int64_t
		{
			e.setTagName("data");

			// set the value
			e.setAttribute("value", getStringFromByteArray(v.toByteArray()));
		}
			break;

		default:
			qWarning("Don't know how to persist variant of type: %s (%d)!",
					 v.typeName(), v.type());
			ok = false;
			break;
	}

	return ok;
}

QString DomConvenience::getStringFromByteArray(const QByteArray& ba)
{
	const char* digitSet = "0123456789abcdef";
	QString data = "";

	for (int i = 0; i < ba.size(); i++)
	{
		char c = ba[i];

		data += digitSet[(c >> 4) & 0xf];
		data += digitSet[c & 0xF];
	}
	return data;
}

QByteArray DomConvenience::getByteArrayFromString(const QString& value)
{
	const QChar* ptr = value.unicode();
	uint32_t len = value.length();

	if (!ptr)
	{
		return QByteArray(0);
	}

	QByteArray data(len / 2, '\0');
	uint8_t dv = 0; uint32_t i = 0;
	bool even = false;
	while (len > 0 && ok_in_hex(*ptr))
	{
		len = len - 1;
		if (even)
			dv = dv << 4;
		else
			dv = 0;

		if (ptr->isDigit())
		{
			dv += ptr->digitValue();
		}
		else
		{
			if (*ptr >= 'a' && *ptr <= 'f')
				dv += ptr->toAscii() - 'a' + 10;
			else if (*ptr >= 'A' && *ptr <= 'F')
				dv += ptr->toAscii() - 'A' + 10;
		}

		if (even)
			data[i++] = dv;
		even = !even;
		ptr++;
	}
	return data;
}

bool DomConvenience::getBoolFromString(const QString& s, bool& ok)
{
	bool val = false;
	if ((s == "true") || (s == "1"))
	{
		val = true;
		ok = true;
	}
	else if ((s == "false") || (s == "0"))
		ok = true;

	return val;
}

int DomConvenience::getBase(const QDomElement& e)
{
	int base = 10;
	bool baseOk = false;
	if (e.hasAttribute("base"))
	{
		base  = e.attribute("base").toInt(&baseOk);
		if (!baseOk)
			base = 10;
	}

	return base;
}

QColor DomConvenience::getColor(const QDomElement& e)
{
	QColor color;
	if (e.hasAttribute("name"))
		color = QColor(e.attribute("name"));

	// allow specifying of base color by name and then tweaking
	if (e.hasAttribute("red") ||
		e.hasAttribute("green") ||
		e.hasAttribute("blue"))
	{
		int base = getBase(e);
		bool colorOk = false;

		// default to black
		int r = 0, g = 0, b = 0;

		// if color was specified by name, use it's RGB values as defaults
		if (color.isValid())
		{
			r = color.red();
			g = color.green();
			b = color.blue();
		}

		if (e.hasAttribute("red"))
			r = e.attribute("red").toInt(&colorOk, base);
		if (e.hasAttribute("green"))
			g = e.attribute("green").toInt(&colorOk, base);
		if (e.hasAttribute("blue"))
			b = e.attribute("blue").toInt(&colorOk, base);

		color = QColor(r, g, b);
	}

	return color;
}

QString DomConvenience::boolString(bool b)
{
	return b ? QString("true") : QString("false");
}

void DomConvenience::clearAttributes(QDomElement& e)
{
	QDomNamedNodeMap attrMap = e.attributes();
	QDomNode attrNode;

	for (uint i = attrMap.length(); i > 0; --i)
		e.removeAttributeNode(attrMap.item(i - 1).toAttr());
}
