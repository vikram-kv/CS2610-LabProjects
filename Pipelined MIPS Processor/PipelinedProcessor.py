'''
File: Pipelined Processor Simulator - Lab 8 Group Assignment
Course: CS2610
Authors: K V Vikram(CS19B021), Anirudh Kulkarni(CS19B075) and Nabyendu Saha(CS19B076)
Inputs : from input folder: ICache.txt, DCache.txt, RF.txt
Outputs: to output folder(pre-existing): DCache.txt, Output.txt
Assumptions: Overflow is not detected nor handled. All instructions/assumptions in the problem
			 statement are strictly followed.
'''

import os

#main class of the processor
class processor:

	def __init__(self, DCache, ICache, RF, buffer4=[], buffer3=[], buffer2=[], buffer1=[], 
				 PC = 0, time = 0, flags = [True,False,False,False,False],
				 LMD = 'NA', R1 = 'NA', A = 'NA', B = 'NA', IR = 'NA', offset = 'NA',
				 L1 = 'NA', ALUOutput = 0, halt = False, aInsn = 0, lInsn = 0, dInsn = 0,
				 cInsn = 0, hInsn = 0, stall = False, cFlag = False, recTime = -10,
				 dStalls = 0, cStalls = 0, regFlags = [False for i in range(16)]):

		self.PC=PC                  # The program counter 
		self.time=time              # clock time set to 0
		self.flags=flags 			# an array of activaion flags for each pipeline stage
		self.LMD=LMD                # load memory data register
		self.R1=R1                  # register 1 (dest register)
		self.A=A                    # operand 1
		self.B=B 					# operand 2
		self.IR=IR 					# Instruction register
		self.offset=offset          # offset for load/store insn. = 4 bit(1 hex digit) signed data 
		self.L1=L1                  # pc offset (in no of insns) for control insns. = 8 bit(2 hex digit) signed data
		self.ALUOutput=ALUOutput    # stores ALU output
		self.halt=halt              # boolean to denote halt. If True, then pipeline is halted
		self.aInsn=aInsn            # arithmetic instructions count
		self.lInsn=lInsn            # logical instructions count
		self.dInsn=dInsn            # data transfer instructions count
		self.cInsn=cInsn            # control transfer instructions count
		self.hInsn=hInsn            # halt instructions count
		self.stall=stall            # boolean is True if pipeline has to be stalled
		self.cFlag=cFlag            # flag to indicate a control stall
		self.recTime=recTime		# time when a control stall was initiated
		self.dStalls=dStalls		# data stalls counter
		self.cStalls=cStalls		# control stalls counter
		self.regFlags=regFlags      # flags associated with each register, True if used currently
		self.DCache=DCache          # data cache read from file
		self.ICache=ICache          # instruction cache read from file
		self.RF=RF                  # register file
		self.buffer1=buffer1        # buffer after stage 1 (instruction fetch stage)
		self.buffer2=buffer2        # buffer after stage 2 (instruction decode stage)
		self.buffer3=buffer3        # buffer after stage 3 (execute stage)
		self.buffer4=buffer4        # buffer after stage 4 (Memory access stage)

	# func to convert 2's complement form to its integer equivalent
	# 2's complement form is given as a hex string 'value'
	def Int(self, value,length = 8):             
		mag = int(value,16)
		length = 4*len(value)
		if(mag > 2**(length-1)-1):    # negative values are detected and converted
			return (-(2**length-mag))
		else:
			return mag 				  # positive values are directly returned

	def Str(self, value):				# converts value to its 2's complement form returned as hex string of length 2
		if(value < 0):
			value = 256 + value         # if negative, we add 256 to conv to 2's complement form
		hexRep = '{0:x}'.format(value)
		if(len(hexRep)==1):				# adding a leading zero for single digit hex numbers
			hexRep = '0' + hexRep
		return hexRep[-2:]

	def ReadData(self, array, address): # returns the 1B data(2 hex char) at 'address' of the string array 'array'
		return array[address][0:2]

	def ReadInsn(self, array, address): # returns the 2B instruction(4 hex char) at 'address' and 'address'+1 positions of 'array'
		return array[address][0:2] + array[address+1][0:2]

	def WriteData(self, array, address, value): # writes 'value' at 'address' of the string array 'array'
		hexRep = self.Str(value)
		array[address] = hexRep + '\n'

	def write_back(self):                # write back step of processor (WB)
		self.flags[4] = False            # reset the WB's activation flag
		# read contents from buffer4
		opCode = self.buffer4[0]
		self.R1 = self.buffer4[1]  
		self.ALUOutput = self.buffer4[2] 
		self.LMD = self.buffer4[3]

		# write data to appropriate destination register in RF depending on the opcode for Arith/Logic op
		if(opCode < 8): 
			self.WriteData(self.RF,int(self.R1,16),self.ALUOutput)
			self.regFlags[int(self.R1,16)] = False		# free the dest register for opnd fetching in IF stage

		# for load instruction
		elif(opCode == 8):
			self.WriteData(self.RF,int(self.R1,16),self.Int(self.LMD))
			self.regFlags[int(self.R1,16)] = False		# free the dest register for opnd fetching in IF stage

		# for halt instruction
		elif(opCode == 15):
			self.halt = True
		return

	def memory_access(self):   		# memory access stage of pipeline (MEM)
		self.flags[3] = False  		# reset MEM's activation flag
		# read all data from buffer3
		opCode = self.buffer3[0]
		self.R1 = self.buffer3[1]    
		self.ALUOutput = self.buffer3[2]

		# for load instruction
		if(opCode == 8):
			self.LMD = self.ReadData(self.DCache,self.ALUOutput)

		# for store instruction
		elif(opCode == 9):
			self.WriteData(self.DCache,self.ALUOutput,self.Int(self.R1))

		self.flags[4] = True	# activate WB stage in next cycle
		self.buffer4 = [opCode,self.R1,self.ALUOutput,self.LMD]		# save reqd info for WB stage in buffer4
		return

	def execute_cycle(self): # execution/ effective address cycle (EX)

		# read necessary inputs from buffer2
		self.flags[2] = False
		opCode = self.buffer2[0]
		self.R1 = self.buffer2[1]
		self.A = self.buffer2[2]
		self.B = self.buffer2[3]
		self.offset = self.buffer2[4]
		self.L1 = self.buffer2[5]

		# based on opcode, perform operation
		if(opCode <= 8):
			self.regFlags[int(self.R1,16)] = True  # indicate register un-available for IF stage's operand fetching
		#stmts to do arithmetic and logic operations after conv opnds to signed integers using Int()
		if(opCode == 0):
			self.aInsn = self.aInsn + 1
			self.ALUOutput = self.Int(self.A) + self.Int(self.B)
		elif(opCode == 1):
			self.aInsn = self.aInsn + 1
			self.ALUOutput = self.Int(self.A) - self.Int(self.B)
		elif(opCode == 2):
			self.aInsn = self.aInsn + 1
			self.ALUOutput = self.Int(self.A) * self.Int(self.B)
		elif(opCode == 3):
			self.aInsn = self.aInsn + 1
			self.ALUOutput = self.Int(self.A) + 1
		elif(opCode == 4):
			self.lInsn = self.lInsn + 1
			self.ALUOutput = self.Int(self.A) & self.Int(self.B)
		elif(opCode == 5):
			self.lInsn = self.lInsn + 1
			self.ALUOutput = self.Int(self.A) | self.Int(self.B)
		elif(opCode == 6):
			self.lInsn = self.lInsn + 1
			self.ALUOutput = ~self.Int(self.A)
		elif(opCode == 7):
			self.lInsn = self.lInsn + 1
			self.ALUOutput = self.Int(self.A) ^ self.Int(self.B)

		# for load/store, we have effective address calculation
		elif(opCode <= 9):
			self.dInsn = self.dInsn + 1
			self.ALUOutput = self.Int(self.A) + self.Int(self.offset)

		# for jmp, we have target address calc and setting PC to target address
		elif(opCode == 10):
			self.cInsn = self.cInsn + 1
			self.ALUOutput = self.PC + (self.Int(self.L1)<<1)
			self.PC = self.ALUOutput

		# for beqz, we have target address calc and setting PC to target address iff Int(R1) == 0
		elif(opCode == 11):
			self.cInsn = self.cInsn + 1
			self.ALUOutput = self.PC + (self.Int(self.L1)<<1)
			cond = (self.Int(self.R1)==0)
			if(cond == True):
				self.PC = self.ALUOutput
		self.flags[3] = True								# activate MEM stage in next cycle
		self.buffer3 = [opCode,self.R1,self.ALUOutput]		# saving data of next cycle for MEM stage in buffer3
		return

	def instruction_decode(self): # ID stage

		# read necessary inputs from buffer1 and deactivate this stage's flag for next cycle
		self.flags[1] = False
		self.IR = self.buffer1[0]
		opCode = int(self.IR[0],16)

		# for ADD, SUB and MUL
		if(opCode <= 2):
			self.R1 = self.IR[1]
			if(self.regFlags[int(self.IR[2],16)] == True or self.regFlags[int(self.IR[3],16)] == True): # if one of the operands is not available, we stall the pipeline
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:                      # if not, fetch them from register file
				self.stall = False
				self.A = self.ReadData(self.RF,int(self.IR[2],16))
				self.B = self.ReadData(self.RF,int(self.IR[3],16))
		# for INC
		elif(opCode == 3):
			self.R1 = self.IR[1]
			if(self.regFlags[int(self.IR[1],16)] == True): # if operand not available, stall
				self.stall = True
				self.flags[1] = True
				self.dStalls +=1
			else:                                          # else fetch from register file
				self.stall = False
				self.A = self.ReadData(self.RF,int(self.IR[1],16))

		# for AND, OR, XOR
		elif(opCode <= 5 or opCode == 7):
			self.R1 = self.IR[1]
			if(self.regFlags[int(self.IR[2],16)] == True or self.regFlags[int(self.IR[3],16)] == True): # if operands not available, stall
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:
				self.stall = False
				self.A = self.ReadData(self.RF,int(self.IR[2],16))
				self.B = self.ReadData(self.RF,int(self.IR[3],16))

		# for NOT
		elif(opCode == 6):
			self.R1 = self.IR[1]
			if(self.regFlags[int(self.IR[2],16)] == True): # check if operand is available
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:
				self.stall = False
				self.A = self.ReadData(self.RF,int(self.IR[2],16))

		# for LOAD
		elif(opCode == 8):
			if(self.regFlags[int(self.IR[2],16)] == True): # stall if operand is not present
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:
				self.stall = False
				self.R1 = self.IR[1]
				self.A = self.ReadData(self.RF,int(self.IR[2],16))
				self.offset = self.IR[3]

		# for STORE
		elif(opCode == 9):
			if(self.regFlags[int(self.IR[1],16)] == True or self.regFlags[int(self.IR[2],16)] == True): # stall if an operand is not present
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:
				self.stall = False
				self.R1 = self.ReadData(self.RF,int(self.IR[1],16))
				self.A = self.ReadData(self.RF,int(self.IR[2],16))
				self.offset = self.IR[3]

		# for JUMP
		elif(opCode == 10):
			self.L1 = self.IR[1:3]
			self.stall = True
			self.cFlag = True		 # control flag is set to indicate a control stall
			self.cStalls += 2        # 2 stalls for fetch and decode of next ins
			self.recTime = self.time

		# for BEQZ
		elif(opCode == 11):
			if(self.regFlags[int(self.IR[1],16)] == True): # check if operand is available and stall accordingly
				self.stall = True
				self.flags[1] = True
				self.dStalls += 1
			else:
				self.R1 = self.ReadData(self.RF,int(self.IR[1],16))
				self.L1 = self.IR[2:4]
				self.stall = True
				self.cFlag = True		# control flag is set to indicate a control stall
				self.cStalls += 2		# 2 stalls for fetch and decode of next ins
				self.recTime = self.time

		#for HLT
		elif(opCode == 15):
			self.flags[0] = False			# IF stage is deactivated from this clock cycle onwards

		# in next cycle, EX stage will be activated if there is no stall or if the stall is a control stall
		if(not self.stall or self.cFlag):
			self.flags[2] = True		# activate EX stage in next clock cycle
			self.buffer2 = [opCode,self.R1,self.A,self.B,self.offset,self.L1]	#reqd data for EX stage is buffered
		return

	def IF_stage(self):
		# insn at PC (2 bytes = hex string of len 4) is fetched from ICache
		# PC is set to point to next insn
		# insn is placed in buffer of ID stage(buffer1)
		# flag of IF is set iff fetched insn is not HLT
		self.IR = self.ReadInsn(self.ICache,self.PC)
		self.PC += 2
		self.flags[1] = True		# activate ID stage for next clock cycle
		self.buffer1 = [self.IR]
		self.flags[0] = True		# activate IF stage for next clock cycle
		return

	def process(self):

		while(True):
			# each stage (except IF) is activated iff its flag is set.
			# IF stage is activated iff its flag is set and stall flag is reset 
			if(self.flags[4]):
				self.write_back()
			if(self.flags[3]):
				self.memory_access()
			if(self.flags[2]):
				self.execute_cycle()
			if(self.flags[1]):
				self.instruction_decode()
			if(self.flags[0] and not self.stall):
				self.IF_stage()
			self.time = self.time + 1   # increment clock time after each stage is visited
			#stmts to release a control stall after 2 clock cycles
			if(self.cFlag  and self.time == self.recTime + 2):
				self.stall = False
				self.cFlag = False

			if(self.halt):  # if halt ins is encountered, we exit from the loop
				self.hInsn += 1
				break

		return

	def write_to_file(self, DPtr, out):					#DPtr, out - file ptr of DCache.txt and output.txt resp
		# write the information to output files
		DPtr.writelines(DCache)
		tInsn = self.aInsn+self.lInsn+self.dInsn+self.cInsn+self.hInsn  # total ins count
		cpi = self.time/tInsn											# average cpi
		tStalls = self.dStalls + self.cStalls                           # total stalls
		# writing reqd data to output file
		out.write('Total number of instructions executed: {}\n'.format(tInsn))
		out.write('Number of instructions in each class     \n')
		out.write('Arithmetic instructions              : {}\n'.format(self.aInsn))
		out.write('Logical instructions                 : {}\n'.format(self.lInsn))
		out.write('Data instructions                    : {}\n'.format(self.dInsn))
		out.write('Control instructions                 : {}\n'.format(self.cInsn))
		out.write('Halt instructions                    : {}\n'.format(self.hInsn))
		out.write('Cycles Per Instruction               : %.5f\n' %cpi )	
		out.write('Total number of stalls               : {}\n'.format(tStalls))
		out.write('Data stalls (RAW)                    : {}\n'.format(self.dStalls))
		out.write('Control stalls                       : {}\n'.format(self.cStalls))
		return


if __name__=="__main__":

	# opening all input files -- DCache.txt  ICache.txt and RF.txt in input folder
	DPtr =  open('input/DCache.txt','rt',encoding = 'utf-8')
	IPtr =  open('input/ICache.txt','rt',encoding = 'utf-8')
	RPtr =  open('input/RF.txt','rt',encoding = 'utf-8')

	# reading each input file into a list of lines for ease of processing
	DCache = DPtr.readlines()
	ICache = IPtr.readlines()
	RF = RPtr.readlines()

	# closing all input files
	DPtr.close()
	IPtr.close()
	RPtr.close()

	# executing the program in our pipelined processor 
	p = processor(DCache=DCache, ICache=ICache, RF=RF)
	p.process()

	# creating and opening all output files -- DCache.txt  Output.txt in output folder
	DPtr = open('output/DCache.txt','wt',encoding = 'utf-8')
	out  = open('output/Output.txt','wt',encoding = 'utf-8')

	# writing to output files 
	p.write_to_file(DPtr=DPtr, out=out)

	# closing all output files
	out.close()
	DPtr.close()
