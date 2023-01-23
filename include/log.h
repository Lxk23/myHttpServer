#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"

using namespace std;

#define LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
	   LogEventWrap(LogEvent::ptr(new LogEvent(logger, level, \
	   __FILE__, __LINE__, 0, Util::GetThreadId(), Util::GetFiberId(), time(0)))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, LogLevel::FATAL)


#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		LogEventWrap(LogEvent::ptr(new LogEvent(logger, level, \
			__FILE__, __LINE__, 0, Util::GetThreadId(), \
		   Util::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() LoggerMgr::getInstance()->getRoot()

//#define LOG_NAME(name) LoggerMgr::getInstance()->getLogger(name)


class Logger;
class LoggerManager;
	
//日志级别
class LogLevel
{
public:
	enum Level{
		UNKOWN = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};
	
	static const char* toString(LogLevel::Level level);
	static LogLevel::Level fromString(const string& str);
};

//用于存放日志的内容
class LogEvent
{
public:
	typedef shared_ptr<LogEvent> ptr;
	LogEvent(shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line,
			uint32_t elapse,uint32_t threadId, uint32_t fiberId, uint64_t time);
	
	const char* getFile() const { return m_file; }
	int32_t getLine() const { return m_line; }
	uint32_t getElapse() const { return m_elapse; }
	uint32_t getThreadId() const { return m_threadId; }
	uint32_t getFiberId() const { return m_fiberId; }
	uint64_t getTime() const { return m_time; }
	string getContent() const { return m_ss.str(); }
	
	shared_ptr<Logger> getLogger() const { return m_logger; }
	LogLevel::Level getLevel() const { return m_level; }

	stringstream& getSS() { return m_ss; }
	void format(const char* fmt, ...);
	void format(const char* fmt, va_list al);
	
private:
	const char* m_file = nullptr; //文件名
	int32_t m_line = 0;           //行号
	uint32_t m_elapse = 0;        //程序启动开始到现在的毫秒数
	uint32_t m_threadId = 0;      //线程号
	uint32_t m_fiberId = 0;       //协程号
	uint64_t m_time;              //时间戳
	stringstream m_ss;             //

	shared_ptr<Logger> m_logger;
	LogLevel::Level m_level;
};	

class LogEventWrap{
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();
	LogEvent::ptr getEvent() const { return m_event; }
	stringstream& getSS();

private:
	LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter
{
public:
	typedef shared_ptr<LogFormatter> ptr;
	LogFormatter(const string& pattern);
	
	string format(shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event); //将event转换为字符串格式，提供给appender输出
public:
	class FormatItem{
	public:
		typedef shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() {}
		virtual void format(ostream& os, shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	};
	
	void init();
	bool isError() const { return m_error; }
	const string getPattern() const { return m_pattern; }
	
private:
	string m_pattern;         //日志格式
	vector<FormatItem::ptr> m_items; //通过日志格式解析出来的FormatItem，支持扩展
	bool m_error = false;	
};

//日志输出地
class LogAppender
{
public:
	typedef shared_ptr<LogAppender> ptr;
	virtual ~LogAppender() {}  //作为基类，使用虚析构
	
	virtual void log(shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	
	LogFormatter::ptr getFormatter() const { return m_formatter; }
	void setFormatter(LogFormatter::ptr format) { m_formatter = format; }

	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level level) { m_level = level; }
	
protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	LogFormatter::ptr m_formatter;
};
	
	
//日志器
class Logger : public std::enable_shared_from_this<Logger>
{
friend class LoggerManager;
public:
	typedef shared_ptr<Logger> ptr;
	
	Logger(const string& name = "root");
	void log(LogLevel::Level level, LogEvent::ptr event);
	
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);
	
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
    void clearAppenders();
	
	LogLevel::Level getLevel() const {return m_level;}
	void setLevel(LogLevel::Level level) {m_level = level;}
	
	const string& getName() const {return m_name;}

    void setFormatter(const string& val);
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

private:
	string m_name;			    //日志名称
	LogLevel::Level m_level;	    //日志级别
	list<LogAppender::ptr> m_appenders; //appender集合
	LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

//输出到控制台的appender
class StdoutLogAppender : public LogAppender
{
public:
	typedef shared_ptr<StdoutLogAppender> ptr;
	virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

//输出到文件的appender
class FileLogAppender : public LogAppender
{
	public:
	typedef shared_ptr<FileLogAppender> ptr;
	FileLogAppender(const string& filename);
	virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
	
	//重新打开文件，文件打开成功时返回true
	bool reopen();
private:
	string m_filename;
	ofstream m_filestream;
	
};

class LoggerManager
{
public:
	LoggerManager();
	Logger::ptr getLogger(const string& name);

	void init();
	Logger::ptr getRoot() const { return m_root; }
private:
	map<string, Logger::ptr> m_loggers;
	Logger::ptr m_root;
};

typedef Singleton<LoggerManager> LoggerMgr;
	

#endif
