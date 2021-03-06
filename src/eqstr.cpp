/*
 * eqstr.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2002-2003 Zaphod (dohpaz@users.sourceforge.net)
 *
 */

#include "pch.h"

#include "eqstr.h"
#include "diagnosticmessages.h"


EQStr::EQStr()
{
	m_loaded = false;
}

EQStr::~EQStr()
{
}

bool EQStr::load(const QString& fileName)
{
	// clear out any existing contents
	m_messageStrings.clear();

	// create a QFile on the file
	QFile formatFile(fileName);

	// open the file read only
	if (!formatFile.open(QIODevice::ReadOnly))
	{
		seqWarn("EQStr: Failed to open '%s'", fileName.latin1());
		return false;
	}

	// allocate a buffer large enough to hold the entire file
	QByteArray textData(formatFile.size() + 1);

	// read in the entire file
	formatFile.readBlock(textData.data(), textData.size());

	// construct a regex to deal with either style line termination
	QRegExp lineTerm("[\r\n]{1,2}");

	// split the data into lines at the line termination
	QStringList lines = QStringList::split(lineTerm, QString::fromUtf8(textData), false);

	// start iterating over the lines
	QStringList::Iterator it = lines.begin();

	// first is the magic id string
	QString magicString = (*it++);
	int spc;
	uint32_t formatId;
	QString formatString;
	uint32_t maxFormatId = 0;

	// next skip over the count, etc...
	it++;

	// now iterate over the format lines
	for (; it != lines.end(); ++it)
	{
		// find the beginning space
		spc = (*it).find(' ');

		// convert the beginnign of the string to a ULong
		formatId = (*it).left(spc).toULong();

		if (formatId > maxFormatId)
			maxFormatId = formatId;

		// insert the format string into the dictionary.
		m_messageStrings.insert(formatId, (*it).mid(spc+1));
	}

	// note that strings are loaded
	m_loaded = true;

	seqInfo("Loaded %d message strings from '%s' maxFormat=%d",
		m_messageStrings.count(), fileName.latin1(),
		maxFormatId);

	return true;
}

QString EQStr::find(uint32_t formatid) const
{
	return m_messageStrings.value(formatid);
}

QString EQStr::message(uint32_t formatid) const
{
	// attempt to find the message string
	if (m_messageStrings.contains(formatid))
		return m_messageStrings[formatid];
	
	// otherwise return a fabricated string
	return QString("Unknown: ") + QString::number(formatid, 16);
}

QString EQStr::formatMessage(uint32_t formatid, const char* arguments, size_t argsLen) const
{
	QString tempStr;

	if (!m_messageStrings.contains(formatid))
	{
		tempStr.sprintf("Unknown: %04x: ", formatid);
		tempStr += QString::fromUtf8(arguments);

		size_t totalArgsLen = strlen(arguments) + 1;

		const char* curMsg;
		while (totalArgsLen < argsLen)
		{
			curMsg = arguments + totalArgsLen;
			tempStr += QString(", ") + QString::fromUtf8(curMsg);
			totalArgsLen += strlen(curMsg) + 1;
		}
	}
	else
	{
		QVector<QString> argList;
		argList.reserve(5); // reserve space for 5 elements to handle most common sizes

		size_t totalArgsLen = 0;
		while (totalArgsLen < argsLen)
		{
			const char* curArg = arguments + totalArgsLen;
			
			// insert argument into the argument list
			argList.push_back(QString::fromUtf8(curArg));
			totalArgsLen += strlen(curArg) + 1;
		}

		bool ok, ok2;
		int curPos;
		size_t substArg;
		int substArgValue;
		QString substFormatString;

		////////////////////////////
		// replace template (%T) arguments in formatted string
		QString formatString = m_messageStrings[formatid];
		QRegExp rxt("%T(\\d{1,3})", true, false);

		// find first template substitution
		curPos = rxt.search(formatString, 0);

		while (curPos != -1)
		{
			ok2 = false;
			substArg = rxt.cap(1).toInt(&ok);
			if (ok && (substArg <= argList.size()))
			{
				substArgValue = argList[substArg-1].toInt(&ok2);
			}

			// Find the message and replace, or skip over if not found.
			if (ok2 && m_messageStrings.contains(substArgValue))
				formatString.replace(curPos, rxt.matchedLength(), m_messageStrings[substArgValue]);
			else
				curPos += rxt.matchedLength();

			// find next substitution
			curPos = rxt.search(formatString, curPos);
		}

		////////////////////////////
		// now replace substitution arguments in formatted string
		// NOTE: not using QString::arg() because not all arguments are always used
		//       and it will do screwy stuff in this situation
		QRegExp rx("%(\\d{1,3})", true, false);

		// find first template substitution
		curPos = rx.search(formatString, 0);

		while (curPos != -1)
		{
			substArg = rx.cap(1).toInt(&ok);

			// replace substitution argument with argument from list
			if (ok && (substArg <= argList.size()))
				formatString.replace(curPos, rx.matchedLength(), argList[substArg-1]);
			else
				curPos += rx.matchedLength(); // if no such argument, skip over

			// find next substitution
			curPos = rx.search(formatString, curPos);
		}

		return formatString;
	}

	return tempStr;
}
