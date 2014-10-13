#pragma once

#ifndef X_PREPROCESSORDECREMENT_H_
#define X_PREPROCESSORDECREMENT_H_


#define X_PP_DECREMENT_0			-1
#define X_PP_DECREMENT_1			0
#define X_PP_DECREMENT_2			1
#define X_PP_DECREMENT_3			2
#define X_PP_DECREMENT_4			3
#define X_PP_DECREMENT_5			4
#define X_PP_DECREMENT_6			5
#define X_PP_DECREMENT_7			6
#define X_PP_DECREMENT_8			7
#define X_PP_DECREMENT_9			8
#define X_PP_DECREMENT_10			9
#define X_PP_DECREMENT_11			10
#define X_PP_DECREMENT_12			11
#define X_PP_DECREMENT_13			12
#define X_PP_DECREMENT_14			13
#define X_PP_DECREMENT_15			14
#define X_PP_DECREMENT_16			15
#define X_PP_DECREMENT_17			16
#define X_PP_DECREMENT_18			17
#define X_PP_DECREMENT_19			18
#define X_PP_DECREMENT_20			19
#define X_PP_DECREMENT_21			20
#define X_PP_DECREMENT_22			21
#define X_PP_DECREMENT_23			22
#define X_PP_DECREMENT_24			23
#define X_PP_DECREMENT_25			24
#define X_PP_DECREMENT_26			25
#define X_PP_DECREMENT_27			26
#define X_PP_DECREMENT_28			27
#define X_PP_DECREMENT_29			28
#define X_PP_DECREMENT_30			29
#define X_PP_DECREMENT_31			30
#define X_PP_DECREMENT_32			31
#define X_PP_DECREMENT_33			32
#define X_PP_DECREMENT_34			33
#define X_PP_DECREMENT_35			34
#define X_PP_DECREMENT_36			35
#define X_PP_DECREMENT_37			36
#define X_PP_DECREMENT_38			37
#define X_PP_DECREMENT_39			38
#define X_PP_DECREMENT_40			39
#define X_PP_DECREMENT_41			40
#define X_PP_DECREMENT_42			41
#define X_PP_DECREMENT_43			42
#define X_PP_DECREMENT_44			43
#define X_PP_DECREMENT_45			44
#define X_PP_DECREMENT_46			45
#define X_PP_DECREMENT_47			46
#define X_PP_DECREMENT_48			47
#define X_PP_DECREMENT_49			48
#define X_PP_DECREMENT_50			49
#define X_PP_DECREMENT_51			50
#define X_PP_DECREMENT_52			51
#define X_PP_DECREMENT_53			52
#define X_PP_DECREMENT_54			53
#define X_PP_DECREMENT_55			54
#define X_PP_DECREMENT_56			55
#define X_PP_DECREMENT_57			56
#define X_PP_DECREMENT_58			57
#define X_PP_DECREMENT_59			58
#define X_PP_DECREMENT_60			59
#define X_PP_DECREMENT_61			60
#define X_PP_DECREMENT_62			61
#define X_PP_DECREMENT_63			62


/// \def X_PP_DECREMENT
/// \ingroup Preprocessor
/// \brief Decrements the given argument by one.
/// \code
///   // outputs 49
///   X_PP_DECREMENT(50)
/// \endcode
#define X_PP_DECREMENT(n)			X_PP_JOIN_2(X_PP_DECREMENT_, n)


#endif // X_PREPROCESSORDECREMENT_H_
