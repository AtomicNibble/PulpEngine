#pragma once

#ifndef X_FLAGS_H_
#define X_FLAGS_H_

// #include <stdio.h>


// X_NAMESPACE_BEGIN(core)

/// \ingroup Util
/// \brief An utility class that vastly extends the usability and debugability of standard C++ enums.
/// \details C++ enums are often used to denote that certain values can be combined using bitwise-operations, resulting
/// in so-called flags. The following is a familiar example of an C++ enum used to denote different file modes:
/// \code
///   enum Mode
///   {
///     MODE_READ = 1 << 0,
///     MODE_WRITE = 1 << 1,
///     MODE_APPEND = 1 << 2,
///     MODE_RANDOM_ACCESS = 1 << 3
///   };
///
///   // values correspond to exactly one bit, and can be combined using bitwise-OR
///   int mode = MODE_READ | MODE_WRITE | MODE_RANDOM_ACCESS;
///
///   // testing for a single mode can be done using bitwise-AND
///   const bool isReadMode = ((mode & MODE_READ) != 0);
/// \endcode
/// Although enums are often used in such circumstances, problems often arise sooner or later, especially when passing
/// values to functions:
/// - If we pass the value as \a Mode, client code cannot use a bitwise-OR to combine values because they are no longer
/// of type \a Mode.
/// - If we pass the value as an integer, client code can now mix values from completely different enums, e.g. \a Mode
/// and \a Fruit, which does not make sense.
///
/// Furthermore, using enums/integers as flags severely complicates debugging. An enum value will only show as a simple
/// integer in the debugger, so it is up to the programmer to decipher to which flags an integer value corresponds to.
///
/// This is where the Flags template class comes into play. Firstly, it solves the problem of parameter-passing because
/// it is its own type which only stores an integer internally, and provides all the operators needed to make it behave
/// like an ordinary integer. Secondly, by cleverly using a helper-struct inside an anonymous union that aliases the
/// integer value, the debugger is able to show values for individual bits, each corresponding to a single flag:
/// \code
///   enum Mode
///   {
///     MODE_READ = 1 << 0,
///     MODE_WRITE = 1 << 1,
///     MODE_APPEND = 1 << 2,
///     MODE_RANDOM_ACCESS = 1 << 3
///   };
///
///   // helper structure used for aliasing the integer storing the flags
///   struct Bits
///   {
///     uint32_t MODE_READ : 1;
///     uint32_t MODE_WRITE : 1;
///     uint32_t MODE_APPEND : 1;
///     uint32_t MODE_RANDOM_ACCESS : 1;
///   };
///
///   // this is stored inside the Flags template class.
///   // note that because we are using an union, the size of the class does not increase.
///   union
///   {
///     uint32_t m_flags;
///     Bits m_bits;
///   };
/// \endcode
/// If the Flags class is now used to store flags of type \a Mode, the debugger will show the following:
/// <img src="../../doc/jpg/flags_in_the_debugger.jpg" alt="Invidivual flag values shown by the debugger">
///
/// Last but not least, the Flags class can also be used to turn the stored values into a human-readable string,
/// useful for displaying them in error messages, for logging, etc. For a complete discussion on the topic of enums/Flags,
///
/// In order to use the Flags class, the template parameter T must be a struct that follows the following concepts:
/// - It must have a member <tt>static const unsigned int FLAGS_COUNT</tt> = ...; that denotes the number of flags.
/// - It must contain a nested enum <tt>enum Enum</tt> that declares the individual flags.
/// - It must contain a nested struct <tt>struct Bits</tt> that is used as the helper-struct described above.
/// - It must define a function <tt>static const char* ToString(uint32_t value)</tt> that converts individual flag
/// values into a human-readable string.
///
/// The following shows a complete example of how to define such a struct:
/// \code
///   struct PlayerStatusFlags
///   {
///     static const unsigned int FLAGS_COUNT = 4;
///
///     enum Enum
///     {
///       POISONED = (1u << 0),
///       BLEEDING = (1u << 1),
///       STARVING = (1u << 2),
///       DEAD = (1u << 3)
///     };
///
///     struct Bits
///     {
///       uint32_t POISONED : 1;
///       uint32_t BLEEDING : 1;
///       uint32_t STARVING : 1;
///       uint32_t DEAD : 1;
///     };
///
///     static const char* ToString(uint32_t value)
///     {
///       switch (value)
///       {
///         case POISONED:
///           return "POISONED";
///
///         case BLEEDING:
///           return "BLEEDING";
///
///         case STARVING:
///           return "STARVING";
///
///         case DEAD:
///           return "DEAD";
///
///         default:
///           X_NO_SWITCH_DEFAULT;
///       }
///     }
///   };
/// \endcode
/// Once defined, the struct can then be used in a simple typedef:
/// \code
///   typedef core::Flags<PlayerStatusFlags> PlayerStatus;
/// \endcode
/// Of course, the library offers a macro \ref X_DECLARE_FLAGS that can be used to define such structs, without
/// having to write the code by hand.
/// \remark The class is derived from the template parameter T in order to pull in the enum values from T::Enum.
/// \sa X_DECLARE_FLAGS
template <class T>
class Flags : public T
{
	typedef typename T::Enum Enum;
	typedef typename T::Bits Bits;

public:
	typedef char Description[512];

	// Default constructor, initializes the flags to zero.
	inline Flags(void);

	// \brief Constructor initializing to a certain flag value.
	// \remark The constructor is non-explicit on purpose, in order to make the class behave like an ordinary enum.
	inline Flags(Enum flag);

	// \brief Constructor initializing to an integer value.
	// \remark The constructor is non-explicit on purpose, and is needed to make operator| work globally.
	inline Flags(uint32_t flags);

	inline void Set(Enum flag);

	inline void Remove(Enum flag);
	inline void Clear(void);

	inline bool IsSet(Enum flag) const;
	inline bool IsAnySet(void) const;
	inline bool AreAllSet(void) const;

	// Bitwise-OR operator.
	inline Flags operator|(Flags other) const;

	// Bitwise-OR operator.
	inline Flags& operator|=(Flags other);

	// Bitwise-AND operator.
	inline Flags operator&(Flags other) const;

	// Bitwise-AND operator.
	inline Flags& operator&=(Flags other);

	// Returns the flags' value as integer.
	inline uint32_t ToInt(void) const;

	/// human-readable string, and returns a pointer to the description string.
	const char* ToString(Description& description) const;

	// compare
	inline bool operator==(const Flags other) const;
	inline bool operator!=(const Flags other) const;

private:
	union {
		uint32_t flags_;
		Bits bits_;
	};
};

template <class T>
class Flags8 : public T
{
	typedef typename T::Enum Enum;
	typedef typename T::Bits Bits;

	static const int32_t FLAG_COUNT = T::FLAGS_COUNT;

	static_assert(FLAG_COUNT <= 8, "Flags8 constructed with a flag type containing more than 8 flags");

public:
	typedef char Description[512];
	// Default constructor, initializes the flags to zero.
	inline Flags8(void);

	// \brief Constructor initializing to a certain flag value.
	// \remark The constructor is non-explicit on purpose, in order to make the class behave like an ordinary enum.
	inline Flags8(Enum flag);

	// \brief Constructor initializing to an integer value.
	// \remark The constructor is non-explicit on purpose, and is needed to make operator| work globally.
	inline Flags8(uint8_t flags);

	inline void Set(Enum flag);

	inline void Remove(Enum flag);
	inline void Clear(void);

	inline bool IsSet(Enum flag) const;
	inline bool IsAnySet(void) const;
	inline bool AreAllSet(void) const;

	// Bitwise-OR operator.
	inline Flags8 operator|(Flags8 other) const;

	// Bitwise-OR operator.
	inline Flags8& operator|=(Flags8 other);

	// Bitwise-AND operator.
	inline Flags8 operator&(Flags8 other) const;

	// Bitwise-AND operator.
	inline Flags8& operator&=(Flags8 other);

	// Returns the flags' value as integer.
	inline uint8_t ToInt(void) const;

	/// human-readable string, and returns a pointer to the description string.
	const char* ToString(Description& description) const;


private:
	union {
		uint8_t flags_;
		Bits bits_;
	};
};



#include "Flags.inl"

// X_NAMESPACE_END


#endif // X_FLAGS_H_
