#pragma once

#ifndef X_PREPROCESSORINCREMENT_H
#define X_PREPROCESSORINCREMENT_H


#define X_PP_INCREMENT_0			1
#define X_PP_INCREMENT_1			2
#define X_PP_INCREMENT_2			3
#define X_PP_INCREMENT_3			4
#define X_PP_INCREMENT_4			5
#define X_PP_INCREMENT_5			6
#define X_PP_INCREMENT_6			7
#define X_PP_INCREMENT_7			8
#define X_PP_INCREMENT_8			9
#define X_PP_INCREMENT_9			10
#define X_PP_INCREMENT_10			11
#define X_PP_INCREMENT_11			12
#define X_PP_INCREMENT_12			13
#define X_PP_INCREMENT_13			14
#define X_PP_INCREMENT_14			15
#define X_PP_INCREMENT_15			16
#define X_PP_INCREMENT_16			17
#define X_PP_INCREMENT_17			18
#define X_PP_INCREMENT_18			19
#define X_PP_INCREMENT_19			20
#define X_PP_INCREMENT_20			21
#define X_PP_INCREMENT_21			22
#define X_PP_INCREMENT_22			23
#define X_PP_INCREMENT_23			24
#define X_PP_INCREMENT_24			25
#define X_PP_INCREMENT_25			26
#define X_PP_INCREMENT_26			27
#define X_PP_INCREMENT_27			28
#define X_PP_INCREMENT_28			29
#define X_PP_INCREMENT_29			30
#define X_PP_INCREMENT_30			31
#define X_PP_INCREMENT_31			32
#define X_PP_INCREMENT_32			33
#define X_PP_INCREMENT_33			34
#define X_PP_INCREMENT_34			35
#define X_PP_INCREMENT_35			36
#define X_PP_INCREMENT_36			37
#define X_PP_INCREMENT_37			38
#define X_PP_INCREMENT_38			39
#define X_PP_INCREMENT_39			40
#define X_PP_INCREMENT_40			41
#define X_PP_INCREMENT_41			42
#define X_PP_INCREMENT_42			43
#define X_PP_INCREMENT_43			44
#define X_PP_INCREMENT_44			45
#define X_PP_INCREMENT_45			46
#define X_PP_INCREMENT_46			47
#define X_PP_INCREMENT_47			48
#define X_PP_INCREMENT_48			49
#define X_PP_INCREMENT_49			50
#define X_PP_INCREMENT_50			51
#define X_PP_INCREMENT_51			52
#define X_PP_INCREMENT_52			53
#define X_PP_INCREMENT_53			54
#define X_PP_INCREMENT_54			55
#define X_PP_INCREMENT_55			56
#define X_PP_INCREMENT_56			57
#define X_PP_INCREMENT_57			58
#define X_PP_INCREMENT_58			59
#define X_PP_INCREMENT_59			60
#define X_PP_INCREMENT_60			61
#define X_PP_INCREMENT_61			62
#define X_PP_INCREMENT_62			63
#define X_PP_INCREMENT_63			64


/// \def X_PP_INCREMENT
/// \ingroup Preprocessor
/// \brief Increments the given argument by one.
/// \code
///   // outputs 51
///   X_PP_INCREMENT(50)
/// \endcode
#define X_PP_INCREMENT(n)			X_PP_JOIN_2(X_PP_INCREMENT_, n)


#endif
