#ifndef __DPLVM_H__
#define __DPLVM_H__

#include "stdafx.h"

#ifdef INTERNAL_COMPILE
#	define _DPLExport __declspec(dllexport)
#else
#	define _DPLExport __declspec(dllimport)
#endif

typedef float			num;
typedef int				AddressingMode;
typedef unsigned char	AsmOp;

namespace DPL {

	class _DPLExport DPLVM {

	public: template<typename T> std::string _toString(T value) {
		std::stringstream ss;
		T t = value;
		ss << t;
		std::string sv;
		ss >> sv;

		return sv;
	}

	public: int				IP;
	public: AddressingMode	AM;
	public: int				SP;
	public: int				HP;

	public: static const int MEMORY_SIZE = 128;
	public: num RAM[MEMORY_SIZE];

	public:	static const size_t ASM_STACK_SIZE = 1024;
	public: AsmOp	AsmStackOperator[ASM_STACK_SIZE];
	public: num		AsmStackOperand1[ASM_STACK_SIZE];
	public: num		AsmStackOperand2[ASM_STACK_SIZE];

	public: static const int STRING_TABLE_SIZE = 64;
	public: std::string StringTable[STRING_TABLE_SIZE];

	public: static const int FUNCTION_TABLE_SIZE = 64;
	public: std::string FunctionCallTable[FUNCTION_TABLE_SIZE];

	protected: AddressingMode am1;
	protected: AddressingMode am2;

	public: void	RamHeapMalloc(int size);
	public: num		GetRamHeapStackUnit(int offset);
	public: void	SetRamHeapStackUnit(int offset, num value);
	public: void	RamStackPush(num val);
	public: num		RamStackPop();
	public: void	RamStackClear();
	public: int		RamStackSize(void);
	public: bool	RamStackEmpty(void);

	public: DPLVM();
	public: void Initialize();

	public:		num		PopArgument();
	protected:	num		getRealOperand(AddressingMode addMode, num value);
	protected:	virtual void	call(const std::string &funName, int argCount);

	public: const std::string &PopArgumentAsString();
	public: void ReturnArgument(num value);

	protected:	bool	excuteAsm();
	public:		void	Run();

	public: void Load(const std::string &fileName);

	protected:	virtual void input(void);
	protected:	virtual void output(int argCount);

	};
};

#endif
