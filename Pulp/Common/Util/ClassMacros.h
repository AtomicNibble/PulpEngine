#pragma once

#ifndef X_CLASSMACROS_H_
#define X_CLASSMACROS_H_

// genral shit for classes

#define X_NO_CREATE(className) \
private:                       \
    className(void);           \
    ~className(void)
#define X_NO_COPY(className) className(const className&) = delete
#define X_NO_ASSIGN(className) className& operator=(const className&) = delete
#define X_NO_ASSIGN_VOLATILE(className) className& operator=(const className&) volatile = delete

#define X_NO_MOVE(className) className(className&&) = delete
#define X_NO_MOVE_ASSIGN(className) className& operator=(className&&) = delete

#define X_NO_COPY_MOVE_ALL(className) \
    X_NO_COPY(className); \
    X_NO_ASSIGN(className); \
    X_NO_MOVE(className); \
    X_NO_MOVE_ASSIGN(className)

#endif // !X_CLASSMACROS_H_
