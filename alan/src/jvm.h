/**
 * @file    jvm.h
 * @brief   Definitions for Java Virtual Machine instructions and data types.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#ifndef JVM_H
#define JVM_H

/* Java array types */
typedef enum {
	T_BOOLEAN = 4,
	T_CHAR,
	T_FLOAT,
	T_DOUBLE,
	T_BYTE,
	T_SHORT,
	T_INT,
	T_LONG
} JVMatype;

/* JVM bytecodes */
typedef enum {
	JVM_ALOAD,
	JVM_ARETURN,
	JVM_ASTORE,
	JVM_GETSTATIC,
	JVM_GOTO,
	JVM_IADD,
	JVM_IALOAD,
	JVM_IAND,
	JVM_IASTORE,
	JVM_IDIV,
	JVM_IFEQ,
	JVM_IF_ICMPEQ,
	JVM_IF_ICMPGE,
	JVM_IF_ICMPGT,
	JVM_IF_ICMPLE,
	JVM_IF_ICMPLT,
	JVM_IF_ICMPNE,
	JVM_ILOAD,
	JVM_IMUL,
	JVM_INEG,
	JVM_INVOKESTATIC,
	JVM_INVOKEVIRTUAL,
	JVM_IOR,
	JVM_ISTORE,
	JVM_ISUB,
	JVM_IREM,
	JVM_IRETURN,
	JVM_IXOR,
	JVM_LDC,
	JVM_NEWARRAY,
	JVM_RETURN,
	JVM_SWAP
} Bytecode;

#endif /* JVM_H */
