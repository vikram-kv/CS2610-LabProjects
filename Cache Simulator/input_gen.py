import random as rn

hex_chars='0123456789abcdef'
write_modes='wr'
fname = input("Enter the name of the file:");
f=open(fname,'w')
for i in range(2**12):
	temp=[rn.choice(hex_chars) for  i in range(5)]
	temp='0xabc'+''.join(temp)+' '+rn.choice(write_modes)
	f.write(temp)
	f.write('\n')
f.close()
