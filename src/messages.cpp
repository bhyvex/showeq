/*
 * messages.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2002-2003,2007 Zaphod (dohpaz@users.sourceforge.net)
 *
 */

#include "pch.h"

#include "messages.h"
#include "datetimemgr.h"

//----------------------------------------------------------------------
// initialize statics
Messages* Messages::s_messages = 0;

//----------------------------------------------------------------------
// Messages
Messages::Messages(DateTimeMgr* dateTimeMgr, MessageFilters* messageFilters,
		   QObject* parent, const char* name)
  : QObject(parent, name),
	m_dateTimeMgr(dateTimeMgr),
	m_messageFilters(messageFilters)
{
	if (!s_messages)
		s_messages = this;

	// connect signals for message filter changes
	connect(m_messageFilters, SIGNAL(removed(uint32_t, uint8_t)),
			this, SLOT(removedFilter(uint32_t, uint8_t)));
	connect(m_messageFilters, SIGNAL(added(uint32_t, uint8_t, const MessageFilter&)),
			this, SLOT(addedFilter(uint32_t, uint8_t, const MessageFilter&)));
}

Messages::~Messages()
{
}

void Messages::addMessage(MessageType type, const QString& text, uint32_t color)
{
	// filter the message
	uint32_t filterFlags = m_messageFilters->filterMessage(type, text);

	// create a message entry
	MessageEntry message(type, QDateTime::currentDateTime(),
						 m_dateTimeMgr->updatedDateTime(),
						 text, color, filterFlags);

	// create the message and append it to the end of the list
	m_messages << message;

	// signal that a new message exists
	emit newMessage(message);
}

void Messages::clear()
{
	// clear the messages
	m_messages.clear();

	// signal that the messages have been cleared
	emit cleared();
}

void Messages::removedFilter(uint32_t mask, uint8_t filter)
{
	// filter has been removed, remove its mask from all the messages
	for (int item = 0; item < m_messages.size(); item++)
	{
		m_messages[item].setFilterFlags(m_messages[item].filterFlags() & ~mask);
	}
}

void Messages::addedFilter(uint32_t mask, uint8_t filterid, const MessageFilter& filter)
{
	// filter has been added, filter all messages against it
	for (int item = 0; item < m_messages.size(); item++)
	{
		MessageEntry& message = m_messages[item];
		if (filter.isFiltered(message))
			message.setFilterFlags(message.filterFlags() | mask);
	}
}

#include "moc_messages.cpp"
