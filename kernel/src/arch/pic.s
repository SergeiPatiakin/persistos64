.global pic_remap
.type pic_remap, @function
pic_remap:
	push %rbp
	mov %rsp, %rbp
	sub $0x8, %rsp
	
	// save masks, enabling needed IMR bits

	in $0x21, %al
  	and $0xF8, %eax // Enable IMR bits 0, 1, 2 of master (mask for IRQ 0x0, 0x1, 0x2)
	mov %eax, -8(%rbp)

	in $0xA1, %al
	and $0xF1, %eax // Enable IMR bits 1, 2, 3 of slave (mask for IRQ 0x9, 0xA, 0xB)
	mov %eax, -4(%rbp)

	// starts the initialization sequence (in cascade mode)

	mov $0x11, %al
	out %al, $0x20

	sub %eax, %eax
	out %al, $0x80

	mov $0x11, %al
	out %al, $0xA0

	sub %eax, %eax
	out %al, $0x80

	// ICW2: Master PIC vector offset

	mov $0x20, %al
	out %al, $0x21

	sub %eax, %eax
	out %al, $0x80

	// ICW2: Slave PIC vector offset

	mov $0x28, %al
	out %al, $0xA1

	sub %eax, %eax
	out %al, $0x80

	// ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)

	mov $0x4, %al
	out %al, $0x21

	sub %eax, %eax
	out %al, $0x80

	// ICW3: tell Slave PIC its cascade identity (0000 0010)

	mov $0x2, %al
	out %al, $0xA1

	sub %eax, %eax
	out %al, $0x80

	// ICW4: have the PICs use 8086 mode (and not 8080 mode)

	mov $0x1, %al
	out %al, $0x21

	sub %eax, %eax
	out %al, $0x80

	mov $0x1, %al
	out %al, $0xA1

	sub %eax, %eax
	out %al, $0x80

	// restore saved masks

	mov -8(%rbp), %eax
	out %al, $0x21

	mov -4(%rbp), %eax
	out %al, $0xA1

	leave
	ret

.global pic_send_eoi
.type pic_send_eoi, @function
pic_send_eoi:
	push %rbp
	mov %rsp, %rbp
	sub $0x8, %rsp
	
    mov $0x20, %al

	cmpq $0x08, %rdi
	jl send_eoi_master
	out %al, $0xA0

send_eoi_master:
	out %al, $0x20

	leave
	ret
