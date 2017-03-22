// used for various CPU instruction lists required in emulator

// INSTRUCTION_0  : No operand instruction 
// INSTRUCTION_1  : Single unsigned byte operand instruction 
// INSTRUCTION_1S : Single signed byte operand instruction
// INSTRUCTION_2  : Single word operand instruction

// description, tick count (in cpu clocks), function name, op id, special code
 INSTRUCTION_0( "NOP", 						4,   nop			,	0x00, {}			)	
 INSTRUCTION_2( "LD BC, 0x%04X", 			12,  ld_bc_nn		,	0x01, {}			)
 INSTRUCTION_0( "LD (BC), A", 				8,   ld_bcp_a		,	0x02, {}			)
 INSTRUCTION_0( "INC BC", 					8,   inc_bc			,	0x03, {}			)
 INSTRUCTION_0( "INC B", 					4,   inc_b			,	0x04, {}			)
 INSTRUCTION_0( "DEC B", 					4,   dec_b			,	0x05, {}			)
 INSTRUCTION_1( "LD B, 0x%02X", 			8,   ld_b_n			,	0x06, {}			)
 INSTRUCTION_0( "RLCA", 					8,   rlca			,	0x07, {}			)
 INSTRUCTION_2( "LD (0x%04X), SP", 			10,  ld_nnp_sp		,	0x08, {}			)
 INSTRUCTION_0( "ADD HL, BC", 				8,   add_hl_bc		,	0x09, {}			)
 INSTRUCTION_0( "LD A, (BC)", 				8,   ld_a_bcp		,	0x0a, {}			)
 INSTRUCTION_0( "DEC BC", 					8,   dec_bc			,	0x0b, {}			)
 INSTRUCTION_0( "INC C", 					4,   inc_c			,	0x0c, {}			)
 INSTRUCTION_0( "DEC C", 					4,   dec_c			,	0x0d, {}			)
 INSTRUCTION_1( "LD C, 0x%02X", 			8,   ld_c_n			,	0x0e, {}			)
 INSTRUCTION_0( "RRCA", 					8,   rrca			,	0x0f, {}			)
 INSTRUCTION_1( "STOP", 					4,   stop			,	0x10, {return;}		)
 INSTRUCTION_2( "LD DE, 0x%04X", 			12,  ld_de_nn		,	0x11, {}			)
 INSTRUCTION_0( "LD (DE), A", 				8,   ld_dep_a		,	0x12, {}			)
 INSTRUCTION_0( "INC DE", 					8,   inc_de			,	0x13, {}			)
 INSTRUCTION_0( "INC D", 					4,   inc_d			,	0x14, {}			)
 INSTRUCTION_0( "DEC D", 					4,   dec_d			,	0x15, {}			)
 INSTRUCTION_1( "LD D, 0x%02X", 			8,   ld_d_n			,	0x16, {}			)
 INSTRUCTION_0( "RLA", 						8,   rla			,	0x17, {}			)
 INSTRUCTION_1( "JR 0x%02X", 				8,   jr_n			,	0x18, {}			)
 INSTRUCTION_0( "ADD HL, DE", 				8,   add_hl_de		,	0x19, {}			)
 INSTRUCTION_0( "LD A, (DE)", 				8,   ld_a_dep		,	0x1a, {}			)
 INSTRUCTION_0( "DEC DE", 					8,   dec_de			,	0x1b, {}			)
 INSTRUCTION_0( "INC E", 					4,   inc_e			,	0x1c, {}			)
 INSTRUCTION_0( "DEC E", 					4,   dec_e			,	0x1d, {}			)
 INSTRUCTION_1( "LD E, 0x%02X", 			8,   ld_e_n			,	0x1e, {}			)
 INSTRUCTION_0( "RRA", 						8,   rra			,	0x1f, {}			)
 INSTRUCTION_1( "JR NZ, 0x%02X", 			0,   jr_nz_n		,	0x20, {}			)
 INSTRUCTION_2( "LD HL, 0x%04X", 			12,  ld_hl_nn		,	0x21, {}			)
 INSTRUCTION_0( "LDI (HL), A", 				8,   ldi_hlp_a		,	0x22, {}			)
 INSTRUCTION_0( "INC HL", 					8,   inc_hl			,	0x23, {}			)
 INSTRUCTION_0( "INC H", 					4,   inc_h			,	0x24, {}			)
 INSTRUCTION_0( "DEC H", 					4,   dec_h			,	0x25, {}			)
 INSTRUCTION_1( "LD H, 0x%02X", 			8,   ld_h_n			,	0x26, {}			)
 INSTRUCTION_0( "DAA", 						4,   daa			,	0x27, {}			)
 INSTRUCTION_1( "JR Z, 0x%02X", 			0,   jr_z_n			,	0x28, {}			)
 INSTRUCTION_0( "ADD HL, HL", 				8,   add_hl_hl		,	0x29, {}			)
 INSTRUCTION_0( "LDI A, (HL)", 				8,   ldi_a_hlp		,	0x2a, {}			)
 INSTRUCTION_0( "DEC HL", 					8,   dec_hl			,	0x2b, {}			)
 INSTRUCTION_0( "INC L", 					4,   inc_l			,	0x2c, {}			)
 INSTRUCTION_0( "DEC L", 					4,   dec_l			,	0x2d, {}			)
 INSTRUCTION_1( "LD L, 0x%02X", 			8,   ld_l_n			,	0x2e, {}			)
 INSTRUCTION_0( "CPL", 						4,   cpl			,	0x2f, {}			)
 INSTRUCTION_1S("JR NC, 0x%02X", 			8,   jr_nc_n		,	0x30, {}			)
 INSTRUCTION_2( "LD SP, 0x%04X", 			12,  ld_sp_nn		,	0x31, {}			)
 INSTRUCTION_0( "LDD (HL), A", 				8,   ldd_hlp_a		,	0x32, {}			)
 INSTRUCTION_0( "INC SP", 					8,   inc_sp			,	0x33, {}			)
 INSTRUCTION_0( "INC (HL)", 				12,  inc_hlp		,	0x34, {}			)
 INSTRUCTION_0( "DEC (HL)", 				12,  dec_hlp		,	0x35, {}			)
 INSTRUCTION_1( "LD (HL), 0x%02X", 			12,  ld_hlp_n		,	0x36, {}			)
 INSTRUCTION_0( "SCF", 						4,   scf			,	0x37, {}			)
 INSTRUCTION_1S("JR C, 0x%02X", 			0,   jr_c_n			,	0x38, {}			)
 INSTRUCTION_0( "ADD HL, SP", 				8,   add_hl_sp		,	0x39, {}			)
 INSTRUCTION_0( "LDD A, (HL)", 				8,   ldd_a_hlp		,	0x3a, {}			)
 INSTRUCTION_0( "DEC SP", 					8,   dec_sp			,	0x3b, {}			)
 INSTRUCTION_0( "INC A", 					4,   inc_a			,	0x3c, {}			)
 INSTRUCTION_0( "DEC A", 					4,   dec_a			,	0x3d, {}			)
 INSTRUCTION_1( "LD A, 0x%02X", 			8,   ld_a_n			,	0x3e, {}			)
 INSTRUCTION_0( "CCF", 						4,   ccf			,	0x3f, {}			)
 INSTRUCTION_0( "LD B, B", 					4,   nop			,	0x40, {}			)
 INSTRUCTION_0( "LD B, C", 					4,   ld_b_c			,	0x41, {}			)
 INSTRUCTION_0( "LD B, D", 					4,   ld_b_d			,	0x42, {}			)
 INSTRUCTION_0( "LD B, E", 					4,   ld_b_e			,	0x43, {}			)
 INSTRUCTION_0( "LD B, H", 					4,   ld_b_h			,	0x44, {}			)
 INSTRUCTION_0( "LD B, L", 					4,   ld_b_l			,	0x45, {}			)
 INSTRUCTION_0( "LD B, (HL)", 				8,   ld_b_hlp		,	0x46, {}			)
 INSTRUCTION_0( "LD B, A", 					4,   ld_b_a			,	0x47, {}			)
 INSTRUCTION_0( "LD C, B", 					4,   ld_c_b			,	0x48, {}			)
 INSTRUCTION_0( "LD C, C", 					4,   nop			,	0x49, {}			)
 INSTRUCTION_0( "LD C, D", 					4,   ld_c_d			,	0x4a, {}			)
 INSTRUCTION_0( "LD C, E", 					4,   ld_c_e			,	0x4b, {}			)
 INSTRUCTION_0( "LD C, H", 					4,   ld_c_h			,	0x4c, {}			)
 INSTRUCTION_0( "LD C, L", 					4,   ld_c_l			,	0x4d, {}			)
 INSTRUCTION_0( "LD C, (HL)", 				8,   ld_c_hlp		,	0x4e, {}			)
 INSTRUCTION_0( "LD C, A", 					4,   ld_c_a			,	0x4f, {}			)
 INSTRUCTION_0( "LD D, B", 					4,   ld_d_b			,	0x50, {}			)
 INSTRUCTION_0( "LD D, C", 					4,   ld_d_c			,	0x51, {}			)
 INSTRUCTION_0( "LD D, D", 					4,   nop			,	0x52, {}			)
 INSTRUCTION_0( "LD D, E", 					4,   ld_d_e			,	0x53, {}			)
 INSTRUCTION_0( "LD D, H", 					4,   ld_d_h			,	0x54, {}			)
 INSTRUCTION_0( "LD D, L", 					4,   ld_d_l			,	0x55, {}			)
 INSTRUCTION_0( "LD D, (HL)", 				8,   ld_d_hlp		,	0x56, {}			)
 INSTRUCTION_0( "LD D, A", 					4,   ld_d_a			,	0x57, {}			)
 INSTRUCTION_0( "LD E, B", 					4,   ld_e_b			,	0x58, {}			)
 INSTRUCTION_0( "LD E, C", 					4,   ld_e_c			,	0x59, {}			)
 INSTRUCTION_0( "LD E, D", 					4,   ld_e_d			,	0x5a, {}			)
 INSTRUCTION_0( "LD E, E", 					4,   nop			,	0x5b, {}			)
 INSTRUCTION_0( "LD E, H", 					4,   ld_e_h			,	0x5c, {}			)
 INSTRUCTION_0( "LD E, L", 					4,   ld_e_l			,	0x5d, {}			)
 INSTRUCTION_0( "LD E, (HL)", 				8,   ld_e_hlp		,	0x5e, {}			)
 INSTRUCTION_0( "LD E, A", 					4,   ld_e_a			,	0x5f, {}			)
 INSTRUCTION_0( "LD H, B", 					4,   ld_h_b			,	0x60, {}			)
 INSTRUCTION_0( "LD H, C", 					4,   ld_h_c			,	0x61, {}			)
 INSTRUCTION_0( "LD H, D", 					4,   ld_h_d			,	0x62, {}			)
 INSTRUCTION_0( "LD H, E", 					4,   ld_h_e			,	0x63, {}			)
 INSTRUCTION_0( "LD H, H", 					4,   nop			,	0x64, {}			)
 INSTRUCTION_0( "LD H, L", 					4,   ld_h_l			,	0x65, {}			)
 INSTRUCTION_0( "LD H, (HL)", 				8,   ld_h_hlp		,	0x66, {}			)
 INSTRUCTION_0( "LD H, A", 					4,   ld_h_a			,	0x67, {}			)
 INSTRUCTION_0( "LD L, B", 					4,   ld_l_b			,	0x68, {}			)
 INSTRUCTION_0( "LD L, C", 					4,   ld_l_c			,	0x69, {}			)
 INSTRUCTION_0( "LD L, D", 					4,   ld_l_d			,	0x6a, {}			)
 INSTRUCTION_0( "LD L, E", 					4,   ld_l_e			,	0x6b, {}			)
 INSTRUCTION_0( "LD L, H", 					4,   ld_l_h			,	0x6c, {}			)
 INSTRUCTION_0( "LD L, L", 					4,   nop			,	0x6d, {}			)
 INSTRUCTION_0( "LD L, (HL)", 				8,   ld_l_hlp		,	0x6e, {}			)
 INSTRUCTION_0( "LD L, A", 					4,   ld_l_a			,	0x6f, {}			)
 INSTRUCTION_0( "LD (HL), B", 				8,   ld_hlp_b		,	0x70, {}			)
 INSTRUCTION_0( "LD (HL), C", 				8,   ld_hlp_c		,	0x71, {}			)
 INSTRUCTION_0( "LD (HL), D", 				8,   ld_hlp_d		,	0x72, {}			)
 INSTRUCTION_0( "LD (HL), E", 				8,   ld_hlp_e		,	0x73, {}			)
 INSTRUCTION_0( "LD (HL), H", 				8,   ld_hlp_h		,	0x74, {}			)
 INSTRUCTION_0( "LD (HL), L", 				8,   ld_hlp_l		,	0x75, {}			)
 INSTRUCTION_0( "HALT", 					4,   halt			,	0x76, {return;}		)
 INSTRUCTION_0( "LD (HL), A", 				8,   ld_hlp_a		,	0x77, {}			)
 INSTRUCTION_0( "LD A, B", 					4,   ld_a_b			,	0x78, {}			)
 INSTRUCTION_0( "LD A, C", 					4,   ld_a_c			,	0x79, {}			)
 INSTRUCTION_0( "LD A, D", 					4,   ld_a_d			,	0x7a, {}			)
 INSTRUCTION_0( "LD A, E", 					4,   ld_a_e			,	0x7b, {}			)
 INSTRUCTION_0( "LD A, H", 					4,   ld_a_h			,	0x7c, {}			)
 INSTRUCTION_0( "LD A, L", 					4,   ld_a_l			,	0x7d, {}			)
 INSTRUCTION_0( "LD A, (HL)", 				8,   ld_a_hlp		,	0x7e, {}			)
 INSTRUCTION_0( "LD A, A", 					4,   nop			,	0x7f, {}			)
 INSTRUCTION_0( "ADD A, B", 				4,   add_a_b		,	0x80, {}			)
 INSTRUCTION_0( "ADD A, C", 				4,   add_a_c		,	0x81, {}			)
 INSTRUCTION_0( "ADD A, D", 				4,   add_a_d		,	0x82, {}			)
 INSTRUCTION_0( "ADD A, E", 				4,   add_a_e		,	0x83, {}			)
 INSTRUCTION_0( "ADD A, H", 				4,   add_a_h		,	0x84, {}			)
 INSTRUCTION_0( "ADD A, L", 				4,   add_a_l		,	0x85, {}			)
 INSTRUCTION_0( "ADD A, (HL)", 				8,   add_a_hlp		,	0x86, {}			)
 INSTRUCTION_0( "ADD A", 					4,   add_a_a		,	0x87, {}			)
 INSTRUCTION_0( "ADC B", 					4,   adc_b			,	0x88, {}			)
 INSTRUCTION_0( "ADC C", 					4,   adc_c			,	0x89, {}			)
 INSTRUCTION_0( "ADC D", 					4,   adc_d			,	0x8a, {}			)
 INSTRUCTION_0( "ADC E", 					4,   adc_e			,	0x8b, {}			)
 INSTRUCTION_0( "ADC H", 					4,   adc_h			,	0x8c, {}			)
 INSTRUCTION_0( "ADC L", 					4,   adc_l			,	0x8d, {}			)
 INSTRUCTION_0( "ADC (HL)", 				8,   adc_hlp		,	0x8e, {}			)
 INSTRUCTION_0( "ADC A", 					4,   adc_a			,	0x8f, {}			)
 INSTRUCTION_0( "SUB B", 					4,   sub_b			,	0x90, {}			)
 INSTRUCTION_0( "SUB C", 					4,   sub_c			,	0x91, {}			)
 INSTRUCTION_0( "SUB D", 					4,   sub_d			,	0x92, {}			)
 INSTRUCTION_0( "SUB E", 					4,   sub_e			,	0x93, {}			)
 INSTRUCTION_0( "SUB H", 					4,   sub_h			,	0x94, {}			)
 INSTRUCTION_0( "SUB L", 					4,   sub_l			,	0x95, {}			)
 INSTRUCTION_0( "SUB (HL)", 				8,   sub_hlp		,	0x96, {}			)
 INSTRUCTION_0( "SUB A", 					4,   sub_a			,	0x97, {}			)
 INSTRUCTION_0( "SBC B", 					4,   sbc_b			,	0x98, {}			)
 INSTRUCTION_0( "SBC C", 					4,   sbc_c			,	0x99, {}			)
 INSTRUCTION_0( "SBC D", 					4,   sbc_d			,	0x9a, {}			)
 INSTRUCTION_0( "SBC E", 					4,   sbc_e			,	0x9b, {}			)
 INSTRUCTION_0( "SBC H", 					4,   sbc_h			,	0x9c, {}			)
 INSTRUCTION_0( "SBC L", 					4,   sbc_l			,	0x9d, {}			)
 INSTRUCTION_0( "SBC (HL)", 				8,   sbc_hlp		,	0x9e, {}			)
 INSTRUCTION_0( "SBC A", 					4,   sbc_a			,	0x9f, {}			)
 INSTRUCTION_0( "AND B", 					4,   and_b			,	0xa0, {}			)
 INSTRUCTION_0( "AND C", 					4,   and_c			,	0xa1, {}			)
 INSTRUCTION_0( "AND D", 					4,   and_d			,	0xa2, {}			)
 INSTRUCTION_0( "AND E", 					4,   and_e			,	0xa3, {}			)
 INSTRUCTION_0( "AND H", 					4,   and_h			,	0xa4, {}			)
 INSTRUCTION_0( "AND L", 					4,   and_l			,	0xa5, {}			)
 INSTRUCTION_0( "AND (HL)", 				8,   and_hlp		,	0xa6, {}			)
 INSTRUCTION_0( "AND A", 					4,   and_a			,	0xa7, {}			)
 INSTRUCTION_0( "XOR B", 					4,   xor_b			,	0xa8, {}			)
 INSTRUCTION_0( "XOR C", 					4,   xor_c			,	0xa9, {}			)
 INSTRUCTION_0( "XOR D", 					4,   xor_d			,	0xaa, {}			)
 INSTRUCTION_0( "XOR E", 					4,   xor_e			,	0xab, {}			)
 INSTRUCTION_0( "XOR H", 					4,   xor_h			,	0xac, {}			)
 INSTRUCTION_0( "XOR L", 					4,   xor_l			,	0xad, {}			)
 INSTRUCTION_0( "XOR (HL)", 				8,   xor_hlp		,	0xae, {}			)
 INSTRUCTION_0( "XOR A", 					4,   xor_a			,	0xaf, {}			)
 INSTRUCTION_0( "OR B", 					4,   or_b			,	0xb0, {}			)
 INSTRUCTION_0( "OR C", 					4,   or_c			,	0xb1, {}			)
 INSTRUCTION_0( "OR D", 					4,   or_d			,	0xb2, {}			)
 INSTRUCTION_0( "OR E", 					4,   or_e			,	0xb3, {}			)
 INSTRUCTION_0( "OR H", 					4,   or_h			,	0xb4, {}			)
 INSTRUCTION_0( "OR L", 					4,   or_l			,	0xb5, {}			)
 INSTRUCTION_0( "OR (HL)", 					8,   or_hlp			,	0xb6, {}			)
 INSTRUCTION_0( "OR A", 					4,   or_a			,	0xb7, {}			)
 INSTRUCTION_0( "CP B", 					4,   cp_b			,	0xb8, {}			)
 INSTRUCTION_0( "CP C", 					4,   cp_c			,	0xb9, {}			)
 INSTRUCTION_0( "CP D", 					4,   cp_d			,	0xba, {}			)
 INSTRUCTION_0( "CP E", 					4,   cp_e			,	0xbb, {}			)
 INSTRUCTION_0( "CP H", 					4,   cp_h			,	0xbc, {}			)
 INSTRUCTION_0( "CP L", 					4,   cp_l			,	0xbd, {}			)
 INSTRUCTION_0( "CP (HL)", 					8,   cp_hlp			,	0xbe, {}			)
 INSTRUCTION_0( "CP A", 					4,   cp_a			,	0xbf, {}			)
 INSTRUCTION_0( "RET NZ", 					0,   ret_nz			,	0xc0, {}			)
 INSTRUCTION_0( "POP BC", 					12,  pop_bc			,	0xc1, {}			)
 INSTRUCTION_2( "JP NZ, 0x%04X", 			0,   jp_nz_nn		,	0xc2, {}			)
 INSTRUCTION_2( "JP 0x%04X", 				12,  jp_nn			,	0xc3, {}			)
 INSTRUCTION_2( "CALL NZ, 0x%04X", 			0,   call_nz_nn		,	0xc4, {}			)
 INSTRUCTION_0( "PUSH BC", 					16,  push_bc		,	0xc5, {}			)
 INSTRUCTION_1( "ADD A, 0x%02X", 			8,   add_a_n		,	0xc6, {}			)
 INSTRUCTION_0( "RST 0x00", 				16,  rst_0			,	0xc7, {}			)
 INSTRUCTION_0( "RET Z", 					0,   ret_z			,	0xc8, {}			)
 INSTRUCTION_0( "RET", 						4,   ret			,	0xc9, {}			)
 INSTRUCTION_2( "JP Z, 0x%04X", 			0,   jp_z_nn		,	0xca, {}			)
 INSTRUCTION_1( "CB %02X", 					0,   cb_n			,	0xcb, {}			)
 INSTRUCTION_2( "CALL Z, 0x%04X", 			0,   call_z_nn		,	0xcc, {}			)
 INSTRUCTION_2( "CALL 0x%04X", 				12,  call_nn		,	0xcd, {}			)
 INSTRUCTION_1( "ADC 0x%02X", 				8,   adc_n			,	0xce, {}			)
 INSTRUCTION_0( "RST 0x08", 				16,  rst_08			,	0xcf, {}			)
 INSTRUCTION_0( "RET NC", 					0,   ret_nc			,	0xd0, {}			)
 INSTRUCTION_0( "POP DE", 					12,  pop_de			,	0xd1, {}			)
 INSTRUCTION_2( "JP NC, 0x%04X", 			0,   jp_nc_nn		,	0xd2, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xd3, {}			)
 INSTRUCTION_2( "CALL NC, 0x%04X", 			0,   call_nc_nn		,	0xd4, {}			)
 INSTRUCTION_0( "PUSH DE", 					16,  push_de		,	0xd5, {}			)
 INSTRUCTION_1( "SUB 0x%02X", 				8,   sub_n			,	0xd6, {}			)
 INSTRUCTION_0( "RST 0x10", 				16,  rst_10			,	0xd7, {}			)
 INSTRUCTION_0( "RET C", 					0,   ret_c			,	0xd8, {}			)
 INSTRUCTION_0( "RETI", 					16,  ret_i			,	0xd9, {}			)
 INSTRUCTION_2( "JP C, 0x%04X", 			0,   jp_c_nn		,	0xda, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xdb, {}			)
 INSTRUCTION_2( "CALL C, 0x%04X", 			0,   call_c_nn		,	0xdc, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xdd, {}			)
 INSTRUCTION_1( "SBC 0x%02X", 				8,   sbc_n			,	0xde, {}			)
 INSTRUCTION_0( "RST 0x18", 				16,  rst_18			,	0xdf, {}			)
 INSTRUCTION_1( "LD (0xFF00 + 0x%02X), A",	12,  ld_ff_n_ap		,	0xe0, {}			)
 INSTRUCTION_0( "POP HL", 					12,  pop_hl			,	0xe1, {}			)
 INSTRUCTION_0( "LD (0xFF00 + C), A", 		8,   ld_ff_c_a		,	0xe2, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xe3, {}			)	
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xe4, {}			)
 INSTRUCTION_0( "PUSH HL", 					16,  push_hl		,	0xe5, {}			)
 INSTRUCTION_1( "AND 0x%02X", 				8,   and_n			,	0xe6, {}			)
 INSTRUCTION_0( "RST 0x20", 				16,  rst_20			,	0xe7, {}			)
 INSTRUCTION_1("ADD SP,0x%02X", 			16,  add_sp_n		,	0xe8, {}			)
 INSTRUCTION_0( "JP HL", 					4,   jp_hl			,	0xe9, {}			)
 INSTRUCTION_2( "LD (0x%04X), A", 			16,  ld_nnp_a		,	0xea, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xeb, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xec, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xed, {}			)
 INSTRUCTION_1( "XOR 0x%02X", 				8,   xor_n			,	0xee, {}			)
 INSTRUCTION_0( "RST 0x28", 				16,  rst_28			,	0xef, {}			)
 INSTRUCTION_1( "LD A, (0xFF00 + 0x%02X)",	12,  ld_ff_ap_n		,	0xf0, {}			)
 INSTRUCTION_0( "POP AF", 					12,  pop_af			,	0xf1, {}			)
 INSTRUCTION_0( "LD A, (0xFF00 + C)", 		8,   ld_a_ff_c		,	0xf2, {}			)
 INSTRUCTION_0( "DI", 						4,   di_inst		,	0xf3, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xf4, {}			)
 INSTRUCTION_0( "PUSH AF", 					16,  push_af		,	0xf5, {}			)
 INSTRUCTION_1( "OR 0x%02X", 				8,   or_n			,	0xf6, {}			)
 INSTRUCTION_0( "RST 0x30", 				16,  rst_30			,	0xf7, {}			)
 INSTRUCTION_1( "LD HL, SP+0x%02X", 		12,  ld_hl_sp_n		,	0xf8, {}			)
 INSTRUCTION_0( "LD SP, HL", 				8,   ld_sp_hl		,	0xf9, {}			)
 INSTRUCTION_2( "LD A, (0x%04X)", 			16,  ld_a_nnp		,	0xfa, {}			)
 INSTRUCTION_0( "EI", 						4,   ei				,	0xfb, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xfc, {}			)
 INSTRUCTION_0( "UNKNOWN", 					0,   undefined		,	0xfd, {}			)
 INSTRUCTION_1( "CP 0x%02X", 				8,   cp_n			,	0xfe, {}			)
 INSTRUCTION_0( "RST 0x38", 				16,  rst_38			,	0xff, {}			)