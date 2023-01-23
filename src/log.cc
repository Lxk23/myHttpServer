#include <map>
#include <utility>
#include <iostream>
#include <functional>
#include <time.h>
#include <string.h>
#include "log.h"
	
LogEvent::LogEvent(shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse,
		uint32_t threadId, uint32_t fiberId, uint64_t time)
		: m_file(file),
		  m_line(line),
		  m_elapse(elapse),
		  m_threadId(threadId),
		  m_fiberId(fiberId),
		  m_time(time),
		  m_logger(logger),
	 	  m_level(level) {
}

void LogEvent::format(const char* fmt, ...) {
	va_list al;
	va_start(al, fmt);
	format(fmt, al);
	va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
	char* buf = nullptr;
	int len = vasprintf(&buf, fmt, al);
	if(len != -1) {
		m_ss << string(buf, len);
		free(buf);
	}
}	

/*******LogLevel*******/
const char* LogLevel::toString(LogLevel::Level level){
	switch(level){
#define XX(name) \
	case LogLevel::name: \
		return #name; \
		break;
	
	XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
	XX(FATAL);
#undef XX
	default:
		return "UNKOWN";
	}
	return "UNKOWN";
}


LogLevel::Level LogLevel::fromString(const string& str) {
#define XX(name) \
	if(str == #name) { \
		return LogLevel::name; \
	}
	XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
	XX(FATAL);
    return LogLevel::UNKOWN;
#undef XX

}

/*******LogLevel*******/

/*******LogEventWrap********/
LogEventWrap::LogEventWrap(LogEvent::ptr e)
	:m_event(e) {
}
LogEventWrap::~LogEventWrap(){
	m_event->getLogger()->log(m_event->getLevel(), m_event);
}

stringstream& LogEventWrap::getSS(){
	return m_event->getSS();
}
/*******LogEventWrap********/

class MessageFormatItem : public LogFormatter::FormatItem{
public:
	MessageFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getContent();
	}
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
	LevelFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << LogLevel::toString(level);
	}
};
		

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
	ElapseFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getElapse();
	}
};		

class NameFormatItem : public LogFormatter::FormatItem{
public:
	NameFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getLogger()->getName();
	}
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
	ThreadIdFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getThreadId();
	}
};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
	FiberIdFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getFiberId();
	}
};

class DataTimeFormatItem : public LogFormatter::FormatItem{
public:
	DataTimeFormatItem(const string& format = "%Y-%m-%d %H:%M:%S") :m_format(format){
		if(m_format.empty()){
			m_format = "%Y-%m-%d %H:%M:%S";
		}
	}
	
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		struct tm tm;
		time_t time = event->getTime();
		localtime_r(&time, &tm);
		char buf[64];
		strftime(buf, sizeof(buf), m_format.c_str(), &tm);
		os << buf;
	}
private:
	string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
	FilenameFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getFile();
	}
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
	LineFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os<<event->getLine();
	}
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
	NewLineFormatItem(const string& str = ""){}
	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os<<endl;
	}
};

class StringFormatItem : public LogFormatter::FormatItem{
public:
	StringFormatItem(const string& str) : m_str(str) {}

	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os<<m_str;
	}
private:
	string m_str;
};
	
class TabFormatItem : public LogFormatter::FormatItem{
public:
	TabFormatItem(const string& str = "") {}

	void format(ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
		os<<"\t";
	}
private:
	string m_str;
};
	
	
/*******Logger*******/
Logger::Logger(const string& name)
	: m_name(name), 
	  m_level(LogLevel::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level){
		auto self = shared_from_this();
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders)
                i->log(self, level, event);
        }
        else if(m_root) {
            m_root->log(level, event);
        }
	}
}

void Logger::setFormatter(const string& val) {
    LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->isError()) {
        cout << "Logger setFormatter name=" << m_name << " value=" << val << " invalid formatter" << endl;
        return;
    }

    m_formatter = new_val;
}

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
}

LogFormatter::ptr Logger::getFormatter() {
    return m_formatter;
}
	
void Logger::debug(LogEvent::ptr event) {
	log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
	log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
	log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
	log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
	log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::ptr appender) {
	if(!appender->getFormatter()){
		appender->setFormatter(m_formatter);
	}
	m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	for(auto it=m_appenders.begin(); it!=m_appenders.end(); it++)
	{
		if(appender == *it){
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearAppenders() {
    m_appenders.clear();
}
/*******Logger*******/


/*******StdoutLogAppender*******/
void StdoutLogAppender::log(shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level){
		cout<<m_formatter->format(logger, level, event);
	}
}
/*******StdoutLogAppender*******/


/*******FileLogAppender*******/
FileLogAppender::FileLogAppender(const string& filename) : m_filename(filename) 
{
}

void FileLogAppender::log(shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level)
		m_filestream<<m_formatter->format(logger, level, event);
}


bool FileLogAppender::reopen() {
	if(m_filestream)
		m_filestream.close();
	m_filestream.open(m_filename);

	return !!m_filestream;
}
/*******FileLogAppender*******/


/*******LogFormatter*******/
LogFormatter::LogFormatter(const string& pattern) 
	: m_pattern(pattern) {
	init();
}

string LogFormatter::format(shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	stringstream ss;
	for(auto& i : m_items){
		i->format(ss, logger, level, event);
	}
	return ss.str();
}

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
	//str format type
	vector<tuple<string, string, int>> vec;
	string nstr;
	
	for(size_t i=0; i<m_pattern.size(); ++i){
		if(m_pattern[i] != '%'){
			nstr.append(1, m_pattern[i]);
			continue;
		}
		
		if((i+1) < m_pattern.size()){
			if(m_pattern[i+1] == '%'){
				nstr.append(1, '%');
				continue;
			}
		}
		
		size_t n = i+1;
		int fmt_status = 0;
		size_t fmt_begin = 0;
		
		string str;
		string fmt;
		while(n < m_pattern.size()){
			if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n]!='{' && m_pattern[n]!='}'))
			{
				str = m_pattern.substr(i+1, n-i-1);
				break;
			}

			if(fmt_status == 0){
				if(m_pattern[n] == '{'){
					str = m_pattern.substr(i+1, n-i-1);
					//cout<<"*"<<str<<endl;
					fmt_status = 1;//解析格式
					fmt_begin = n;
					++n;
					continue;
				}
			}
			else if(fmt_status == 1){
				if(m_pattern[n] == '}'){
					fmt = m_pattern.substr(fmt_begin+1, n-fmt_begin-1);
					fmt_status = 0;
					++n;
					break;
				}
			}
			++n;
			if(n == m_pattern.size())
			{
				if(str.empty())
				{
					str = m_pattern.substr(i+1);
				}
			}
		}
		
		if(fmt_status == 0){
			if(!nstr.empty()){
				vec.push_back(make_tuple(nstr, string(), 0));
				nstr.clear();
			}

			vec.push_back(make_tuple(str, fmt, 1));
			i = n-1;
		}
		else if(fmt_status == 1){
			cout<<"pattern parse error:"<<m_pattern<<"-"<<m_pattern.substr(i)<<endl;
            m_error = true;
			vec.push_back(make_tuple("<<pattern_error>>", fmt, 0));
		}
	}
	
	if(!nstr.empty())
		vec.push_back(make_tuple(nstr, "", 0));
	
	//%m -- 消息体
	//%p -- level
	//%r -- 启动后的时间
	//%c -- 日志名称
	//%t -- 线程id
	//%n -- 回车换行
	//%d -- 时间
	//%f -- 文件名
	//%l -- 行号
	static map<string, function<FormatItem::ptr(const string& str)> > s_format_items = {
#define XX(str, C) \
		{#str, [](const string& fmt){return FormatItem::ptr(new C(fmt));}}
		
		XX(m, MessageFormatItem),
		XX(p, LevelFormatItem),
		XX(r, ElapseFormatItem),
		XX(c, NameFormatItem),
		XX(t, ThreadIdFormatItem),
		XX(n, NewLineFormatItem),
		XX(d, DataTimeFormatItem),
		XX(f, FilenameFormatItem),
		XX(l, LineFormatItem),
		XX(T, TabFormatItem),
		XX(F, FiberIdFormatItem),
#undef XX
	};
	
	for(auto& i : vec){
		if(std::get<2>(i) == 0){
			m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
		}
		else{
			auto it = s_format_items.find(std::get<0>(i));
			if(it == s_format_items.end()){
				m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
			}
			else{
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
		
		//cout<<"("<<std::get<0>(i)<<") - ("<<std::get<1>(i)<<") - ("<<std::get<2>(i)<<")"<<endl;
	}
}
/*******LogFormatter*******/


/*******LogManager********/
LoggerManager::LoggerManager() {
	m_root.reset(new Logger);
	m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    init();
}

Logger::ptr LoggerManager::getLogger(const string& name) {
	auto it = m_loggers.find(name);
    if(it !=m_loggers.find(name)) {
        return it->second;
    }
    
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

void LoggerManager::init() {
}
/*******LogManager********/

