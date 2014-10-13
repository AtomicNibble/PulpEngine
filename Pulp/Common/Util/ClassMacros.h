#pragma once

#ifndef X_CLASSMACROS_H_
#define X_CLASSMACROS_H_

// genral shit for classes

#define X_NO_CREATE(className)			private: className(void); ~className(void)
#define X_NO_COPY(className)			private: className(const className&)
#define X_NO_ASSIGN(className)			private: className& operator=(const className&)


#endif // !X_CLASSMACROS_H_
