#include "stdafx.h"
#include "dplvm.h"

namespace DPL {

	// Memory operation
	const AsmOp MOV = 127;
	const AsmOp LAM = 126;
	// Numeric calculation
	const AsmOp ADD = 119;
	const AsmOp SUB = 118;
	const AsmOp MUL = 117;
	const AsmOp DIV = 116;
	const AsmOp MOD = 115;
	// Numeric comparation
	const AsmOp CE = 109;
	const AsmOp CLE = 108;
	const AsmOp CGE = 107;
	const AsmOp CL = 106;
	const AsmOp CG = 105;
	const AsmOp CNE = 104;
	// Boolean operation
	const AsmOp AND = 99;
	const AsmOp OR = 98;
	const AsmOp NOT = 97;
	// Instruction jumping
	const AsmOp JMP = 10;
	const AsmOp JF = 11;
	const AsmOp JT = 12;
	// Others
	const AsmOp HLT = 0;
	const AsmOp CALL = 1;
	const AsmOp ARG = 2;
	const AsmOp HAL = 3;

	const AddressingMode AM_NIL = 0x00000000;	// NULL
	const AddressingMode AM_RAM = 0x00000001;	// RAM
	const AddressingMode AM_STK = 0x00000002;	// Stack
	const AddressingMode AM_IMD = 0x00000004;	// Imd
	const AddressingMode AM_STR = 0x00000008;	// String

	const int SINGLE_LINE_BUFFER_SIZE = 10240;

	int _getInt(std::fstream &fs) {
		int value = -1;
		char* p = (char*)(&value);
		fs.read(p, 4);

		return value;
	}

	AsmOp _getAsmOp(std::fstream &fs) {
		AsmOp value = -1;
		char* p = (char*)(&value);
		fs.read(p, sizeof(AsmOp));

		return value;
	}

	num _getNum(std::fstream &fs) {
		num value = -1;
		char* p = (char*)(&value);
		fs.read(p, sizeof(num));

		return value;
	}

	std::string _getString(std::fstream &fs, int count) {
		static char buf[SINGLE_LINE_BUFFER_SIZE];
		memset(buf, 0, SINGLE_LINE_BUFFER_SIZE);
		fs.read(buf, count);

		return std::string(buf);
	}

	AddressingMode CombineAddressingMode(AddressingMode am1, AddressingMode am2) {
		return (am1 << 16) | am2;
	}

	AddressingMode DepartAddressingMode(AddressingMode am, int index) {
		return (am >> (index * 16)) & 0x0000ffff;
	}

	void DPLVM::RamHeapMalloc(int size) {
		if (size < 0 || size > MEMORY_SIZE) {
			throw new std::exception("Invalid mallocing heap size");
		}
		HP = size - 1;
	}

	void DPLVM::RamStackPush(num val) {
		if (SP - 1 <= HP) {
			throw new std::exception("Memory overflow, failed while pushing to much values into stack");
		}
		RAM[--SP] = val;
	}

	num DPLVM::RamStackPop() {
		if (SP > MEMORY_SIZE - 1) {
			throw new std::exception("Memory overflow, failed while poping from empty stack");
		}
		num val = RAM[SP++];

		return val;
	}

	void DPLVM::RamStackClear() {
		SP = MEMORY_SIZE;
	}

	int DPLVM::RamStackSize(void) {
		return MEMORY_SIZE - SP;
	}

	bool DPLVM::RamStackEmpty(void) {
		return MEMORY_SIZE == SP;
	}

	num DPLVM::GetRamHeapStackUnit(int offset) {
		return RAM[offset];
	}

	void DPLVM::SetRamHeapStackUnit(int offset, num value) {
		RAM[offset] = value;
	}

	DPLVM::DPLVM() {
		am1 = AM_NIL;
		am2 = AM_NIL;
	}

	void DPLVM::Initialize() {
		IP = 0;
		AM = CombineAddressingMode(AM_NIL, AM_NIL);
		SP = MEMORY_SIZE;
		HP = -1;

		for (int i = 0; i < MEMORY_SIZE; i++) {
			RAM[i] = 0;
		}
		for (int i = 0; i < STRING_TABLE_SIZE; i++) {
			StringTable[i] = "";
		}
		for (int i = 0; i < FUNCTION_TABLE_SIZE; i++) {
			FunctionCallTable[i] = "";
		}
	}

	num DPLVM::getRealOperand(AddressingMode addMode, num value) {
		num result = 0xffffffff;
		switch (addMode) {
		case AM_RAM:
			result = GetRamHeapStackUnit((int)value);
			break;
		case AM_STK:
			result = RamStackPop();
			break;
		case AM_IMD:
			result = value;
			break;
		default:
			throw new std::exception("Invalid addressing mode");
		}

		return result;
	}

	num DPLVM::PopArgument() {
		num am = RamStackPop();
		num op2 = RamStackPop();
		num result = getRealOperand((int)am, op2);

		return result;
	}

	const std::string &DPLVM::PopArgumentAsString() {
		num am = RamStackPop();
		num op2 = RamStackPop();
		static std::string result;
		result = "";
		if (am != AM_STR) {
			result = _toString(getRealOperand((int)am, op2));
		}
		else {
			result = StringTable[(int)op2];
		}

		return result;
	}

	void DPLVM::ReturnArgument(num value) {
		num am = RamStackPop();
		num op2 = RamStackPop();
		if ((AddressingMode)am != AM_RAM) {
			throw new std::exception("Invalid arguments addressing mode");
		}
		SetRamHeapStackUnit((int)op2, value);
	}

	bool DPLVM::excuteAsm() {
		if (IP < 0 || IP >= ASM_STACK_SIZE) {
			throw new std::exception("Instruction pointer overflow");
		}
		AsmOp asmType = AsmStackOperator[IP];
		switch (asmType) {
		case LAM: {
			AM = (int)AsmStackOperand1[IP];
			am1 = DepartAddressingMode(AM, 1);
			am2 = DepartAddressingMode(AM, 0);
			IP++;
			break;
		}
		case MOV: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			switch (am2) {
			case AM_RAM:
				SetRamHeapStackUnit((int)AsmStackOperand2[IP], op1);
				break;
			case AM_STK:
				RamStackPush(op1);
				break;
			default:
				throw new std::exception("Invalid addressing mode");
			}
			IP++;
			break;
		}
		case ADD: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = op1 + op2;
			RamStackPush(result);
			IP++;
			break;
		}
		case SUB: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = op1 - op2;
			RamStackPush(result);
			IP++;
			break;
		}
		case MUL: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = op1 * op2;
			RamStackPush(result);
			IP++;
			break;
		}
		case DIV: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = op1 / op2;
			RamStackPush(result);
			IP++;
			break;
		}
		case MOD: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = (int)op1 % (int)op2;
			RamStackPush(result);
			IP++;
			break;
		}
		case CE: {         // ==
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = (op1 == op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case CLE: {        // <=
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = (op1 <= op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case CGE: {        // >=
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = (op1 >= op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case CL: {        // <
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = (op1 < op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case CG: {        // >
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			if (am1 == AM_STK && am2 == AM_STK) {
				std::swap(op1, op2);
			}
			num result = (op1 > op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case CNE: {        // ~=
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = (op1 != op2) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case AND: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = ((op1 != 0) && (op2 != 0)) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case OR: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num op2 = getRealOperand(am2, AsmStackOperand2[IP]);
			num result = ((op1 != 0) || (op2 != 0)) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case NOT: {
			num op1 = getRealOperand(am1, AsmStackOperand1[IP]);
			num result = (op1 == 0) ? 1 : 0;
			RamStackPush(result);
			IP++;
			break;
		}
		case JMP: {
			IP = (int)AsmStackOperand1[IP];
			break;
		}
		case JF: {
			num val = RamStackPop();
			bool cond = val != 0;
			if (cond) {
				IP = (int)AsmStackOperand1[IP];
			}
			else {
				IP++;
			}
			break;
		}
		case JT: {
			num val = RamStackPop();
			bool cond = val != 0;
			if (cond) {
				IP++;
			}
			else {
				IP = (int)AsmStackOperand1[IP];
			}
			break;
		}
		case HLT: {
			return false;
			break;
		}
		case ARG: {
			RamStackPush(AsmStackOperand2[IP]);
			RamStackPush(AsmStackOperand1[IP]);
			IP++;
			break;
		}
		case CALL: {
			std::string &functionName = FunctionCallTable[(int)AsmStackOperand1[IP]];
			int argCount = (int)AsmStackOperand2[IP];
			call(functionName, argCount);
			IP++;
			break;
		}
		case HAL: {
			int size = (int)AsmStackOperand1[IP];
			RamHeapMalloc(size);
			IP++;
			break;
		}
		default:
			throw new std::exception("Unknown instruction");
		}

		return true;
	}

	void DPLVM::Run() {
		IP = 0;
		AM = AM_NIL;
		SP = MEMORY_SIZE;

		am1 = DepartAddressingMode(AM, 1);
		am2 = DepartAddressingMode(AM, 0);
		bool _continue = true;
		while (_continue) {
			_continue = excuteAsm();
		}
	}

	void DPLVM::output(int argCount) {
		std::string r = "";
		for (int i = 0; i < argCount; ++i) {
			r = PopArgumentAsString() + r;
		}
		std::cout << r << std::endl;
	}

	void DPLVM::input(void) {
		num value;
		std::cout << "Please input:";
		std::cin >> value;
		ReturnArgument(value);
	}

	void DPLVM::call(const std::string &funName, int argCount) {
		if (funName == "output") {
			output(argCount);
		}
		else if (funName == "input") {
			input();
		}
		else {
			throw std::exception("Unknown function");
		}
	}

	void DPLVM::Load(const std::string &fileName) {
		std::fstream fs(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
		if (fs.fail()) {
			throw std::exception("Failed to open file");
		}

		for (int i = 0; i < FUNCTION_TABLE_SIZE; i++) {
			int count = _getInt(fs);
			FunctionCallTable[i] = _getString(fs, count);
		}
		for (int i = 0; i < STRING_TABLE_SIZE; i++) {
			int count = _getInt(fs);
			StringTable[i] = _getString(fs, count);
		}
		int _length = _getInt(fs);
		//		asmStack.Clear();
		for (int i = 0; i < _length; i++) {
			AsmOp op = _getAsmOp(fs);
			num op1 = _getNum(fs);
			num op2 = _getNum(fs);

			AsmStackOperator[i] = op;
			AsmStackOperand1[i] = op1;
			AsmStackOperand2[i] = op2;
		}

		fs.close();
	}
};
