#include "logging.hpp"
#include "string.hpp"

using namespace WarGrey::SCADA;

void ISyslogReceiver::log_message(Log level, Platform::String^ message, ISyslogData* data, Platform::String^ topic) {
	if (level >= this->level) {
		if ((this->topic == nullptr) || (this->topic->Equals(topic))) {
			this->on_log_message(level, message, data, topic);
		}
	}
}

/*************************************************************************************************/
Syslog::Syslog(Log level, Platform::String^ topic, Syslog* parent) : level(level), topic(topic) {
	if (parent != nullptr) {
		this->parent = parent;
		this->parent->reference();
	}

	this->receivers = new std::list<ISyslogReceiver*>();
}

Syslog::~Syslog() {
	if (parent != nullptr) {
		this->parent->destroy();
	}

	while (!this->receivers->empty()) {
		this->receivers->front()->destroy();
		this->receivers->pop_front();
	}

	delete this->receivers;
}

void Syslog::append_log_receiver(ISyslogReceiver* receiver) {
	if (receiver != nullptr) {
		receiver->reference();
		this->receivers->push_back(receiver);
	}
}

void Syslog::log_message(WarGrey::SCADA::Log level, Platform::String^ message, Platform::String^ alt_topic, bool prefix) {
	if (level >= this->level) {
		this->do_log_message(level, message, alt_topic, prefix);
	}
}

void Syslog::log_message(WarGrey::SCADA::Log level, const wchar_t* msgfmt, ...) {
	if (level >= this->level) {
		VSWPRINT(message, msgfmt);
		this->do_log_message(level, message, this->topic, true);
	}
}

void Syslog::log_message(WarGrey::SCADA::Log level, Platform::String^ alt_topic, const wchar_t* msgfmt, ...) {
	if (level >= this->level) {
		VSWPRINT(message, msgfmt);
		this->do_log_message(level, message, alt_topic, true);
	}
}

void Syslog::do_log_message(WarGrey::SCADA::Log level, Platform::String^ message, Platform::String^ topic, bool prefix) {
	auto actual_topic = ((topic == nullptr) ? this->topic : topic);
	auto actual_message = (((!prefix) || (actual_topic == nullptr)) ? message : (actual_topic + ": " + message));
	auto logger = this;

	while (logger != nullptr) {
		auto readers = logger->receivers;
		auto no_more = readers->end();

		for (auto it = readers->begin(); it != no_more; it++) {
			(*it)->log_message(level, actual_message, nullptr, actual_topic);
		}

		// TODO: do we need propagated level?
		logger = this->parent;
	}
}