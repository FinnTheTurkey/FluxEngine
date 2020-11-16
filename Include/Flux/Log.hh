/*
Logging and assertions for Engine

*/

#ifndef ENGINE_LOG_H
#define ENGINE_LOG_H

#include <iostream>
#include <string>

#ifndef __EMSCRIPTEN__
#include <term-colors/termcolors.h>
#else
namespace termcolors {
    /**
     * Colors, according to ANSI color escapes.  Bright variants are
     * listed in parentheses when they differ in hue.
     */
    enum class color {
        reset   = 9,            /**< Reset to previous color */
        black   = 0,            /**< Black (Dark Gray) */
        red     = 1,            /**< Red */
        green   = 2,            /**< Green */
        yellow  = 3,            /**< Yellow */
        blue    = 4,            /**< Blue */
        magenta = 5,            /**< Magenta */
        cyan    = 6,            /**< Cyan */
        white   = 7,            /**< White (Pure White) */
    };
}
#endif

// No fancy classes for this one

// Apart from this exception
class AssertFailed: public std::exception
{
    private:
        std::string message;
    public:
        AssertFailed(std::string msg): std::exception()
        {
            message = msg;
        }
        const char* what() const throw() { return message.c_str(); }
};


void __log(const std::string& text, termcolors::color color);

void __logFileLines(const std::string& prefix, const std::string& text, const char* file, int line, termcolors::color color);


#define LOG_SUCCESS(txt) __log(txt, termcolors::color::green)
#define LOG_INFO(txt) __log(txt, termcolors::color::blue)
#define LOG_WARN(txt) __logFileLines("WARNING: ", txt, __FILE__, __LINE__, termcolors::color::yellow)
#define LOG_ERROR(txt) __logFileLines("ERROR: " , txt, __FILE__, __LINE__, termcolors::color::red)

void __logAssert(bool condition, bool crash, const std::string& message, const char* file, int line);

#define LOG_ASSERT_MESSAGE(cond, txt) __logAssert(cond, false, txt, __FILE__, __LINE__)
#define LOG_ASSERT(cond) __logAssert(cond, false, "Value != true", __FILE__, __LINE__)

#define LOG_ASSERT_MESSAGE_FATAL(cond, txt) __logAssert(cond, true, txt, __FILE__, __LINE__)
#define LOG_ASSERT_FATAL(cond) __logAssert(cond, true, "Value != true", __FILE__, __LINE__)


#endif