// Tables use for fast word aligned bit resolution

unsigned int BitResolveTable[256] = {
	0x00000000, 0x00000010, 0x00001000, 0x00001010, 0x00100000, 0x00100010, 0x00101000, 0x00101010,
	0x10000000, 0x10000010, 0x10001000, 0x10001010, 0x10100000, 0x10100010, 0x10101000, 0x10101010,
	0x00000004, 0x00000014, 0x00001004, 0x00001014, 0x00100004, 0x00100014, 0x00101004, 0x00101014,
	0x10000004, 0x10000014, 0x10001004, 0x10001014, 0x10100004, 0x10100014, 0x10101004, 0x10101014,
	0x00000400, 0x00000410, 0x00001400, 0x00001410, 0x00100400, 0x00100410, 0x00101400, 0x00101410,
	0x10000400, 0x10000410, 0x10001400, 0x10001410, 0x10100400, 0x10100410, 0x10101400, 0x10101410,
	0x00000404, 0x00000414, 0x00001404, 0x00001414, 0x00100404, 0x00100414, 0x00101404, 0x00101414,
	0x10000404, 0x10000414, 0x10001404, 0x10001414, 0x10100404, 0x10100414, 0x10101404, 0x10101414,
	0x00040000, 0x00040010, 0x00041000, 0x00041010, 0x00140000, 0x00140010, 0x00141000, 0x00141010,
	0x10040000, 0x10040010, 0x10041000, 0x10041010, 0x10140000, 0x10140010, 0x10141000, 0x10141010,
	0x00040004, 0x00040014, 0x00041004, 0x00041014, 0x00140004, 0x00140014, 0x00141004, 0x00141014,
	0x10040004, 0x10040014, 0x10041004, 0x10041014, 0x10140004, 0x10140014, 0x10141004, 0x10141014,
	0x00040400, 0x00040410, 0x00041400, 0x00041410, 0x00140400, 0x00140410, 0x00141400, 0x00141410,
	0x10040400, 0x10040410, 0x10041400, 0x10041410, 0x10140400, 0x10140410, 0x10141400, 0x10141410,
	0x00040404, 0x00040414, 0x00041404, 0x00041414, 0x00140404, 0x00140414, 0x00141404, 0x00141414,
	0x10040404, 0x10040414, 0x10041404, 0x10041414, 0x10140404, 0x10140414, 0x10141404, 0x10141414,
	0x04000000, 0x04000010, 0x04001000, 0x04001010, 0x04100000, 0x04100010, 0x04101000, 0x04101010,
	0x14000000, 0x14000010, 0x14001000, 0x14001010, 0x14100000, 0x14100010, 0x14101000, 0x14101010,
	0x04000004, 0x04000014, 0x04001004, 0x04001014, 0x04100004, 0x04100014, 0x04101004, 0x04101014,
	0x14000004, 0x14000014, 0x14001004, 0x14001014, 0x14100004, 0x14100014, 0x14101004, 0x14101014,
	0x04000400, 0x04000410, 0x04001400, 0x04001410, 0x04100400, 0x04100410, 0x04101400, 0x04101410,
	0x14000400, 0x14000410, 0x14001400, 0x14001410, 0x14100400, 0x14100410, 0x14101400, 0x14101410,
	0x04000404, 0x04000414, 0x04001404, 0x04001414, 0x04100404, 0x04100414, 0x04101404, 0x04101414,
	0x14000404, 0x14000414, 0x14001404, 0x14001414, 0x14100404, 0x14100414, 0x14101404, 0x14101414,
	0x04040000, 0x04040010, 0x04041000, 0x04041010, 0x04140000, 0x04140010, 0x04141000, 0x04141010,
	0x14040000, 0x14040010, 0x14041000, 0x14041010, 0x14140000, 0x14140010, 0x14141000, 0x14141010,
	0x04040004, 0x04040014, 0x04041004, 0x04041014, 0x04140004, 0x04140014, 0x04141004, 0x04141014,
	0x14040004, 0x14040014, 0x14041004, 0x14041014, 0x14140004, 0x14140014, 0x14141004, 0x14141014,
	0x04040400, 0x04040410, 0x04041400, 0x04041410, 0x04140400, 0x04140410, 0x04141400, 0x04141410,
	0x14040400, 0x14040410, 0x14041400, 0x14041410, 0x14140400, 0x14140410, 0x14141400, 0x14141410,
	0x04040404, 0x04040414, 0x04041404, 0x04041414, 0x04140404, 0x04140414, 0x04141404, 0x04141414,
	0x14040404, 0x14040414, 0x14041404, 0x14041414, 0x14140404, 0x14140414, 0x14141404, 0x14141414
};

unsigned int BitResolveTableRev[256] = {
	0x00000000, 0x04000000, 0x00040000, 0x04040000, 0x00000400, 0x04000400, 0x00040400, 0x04040400,
	0x00000004, 0x04000004, 0x00040004, 0x04040004, 0x00000404, 0x04000404, 0x00040404, 0x04040404,
	0x10000000, 0x14000000, 0x10040000, 0x14040000, 0x10000400, 0x14000400, 0x10040400, 0x14040400,
	0x10000004, 0x14000004, 0x10040004, 0x14040004, 0x10000404, 0x14000404, 0x10040404, 0x14040404,
	0x00100000, 0x04100000, 0x00140000, 0x04140000, 0x00100400, 0x04100400, 0x00140400, 0x04140400,
	0x00100004, 0x04100004, 0x00140004, 0x04140004, 0x00100404, 0x04100404, 0x00140404, 0x04140404,
	0x10100000, 0x14100000, 0x10140000, 0x14140000, 0x10100400, 0x14100400, 0x10140400, 0x14140400,
	0x10100004, 0x14100004, 0x10140004, 0x14140004, 0x10100404, 0x14100404, 0x10140404, 0x14140404,
	0x00001000, 0x04001000, 0x00041000, 0x04041000, 0x00001400, 0x04001400, 0x00041400, 0x04041400,
	0x00001004, 0x04001004, 0x00041004, 0x04041004, 0x00001404, 0x04001404, 0x00041404, 0x04041404,
	0x10001000, 0x14001000, 0x10041000, 0x14041000, 0x10001400, 0x14001400, 0x10041400, 0x14041400,
	0x10001004, 0x14001004, 0x10041004, 0x14041004, 0x10001404, 0x14001404, 0x10041404, 0x14041404,
	0x00101000, 0x04101000, 0x00141000, 0x04141000, 0x00101400, 0x04101400, 0x00141400, 0x04141400,
	0x00101004, 0x04101004, 0x00141004, 0x04141004, 0x00101404, 0x04101404, 0x00141404, 0x04141404,
	0x10101000, 0x14101000, 0x10141000, 0x14141000, 0x10101400, 0x14101400, 0x10141400, 0x14141400,
	0x10101004, 0x14101004, 0x10141004, 0x14141004, 0x10101404, 0x14101404, 0x10141404, 0x14141404,
	0x00000010, 0x04000010, 0x00040010, 0x04040010, 0x00000410, 0x04000410, 0x00040410, 0x04040410,
	0x00000014, 0x04000014, 0x00040014, 0x04040014, 0x00000414, 0x04000414, 0x00040414, 0x04040414,
	0x10000010, 0x14000010, 0x10040010, 0x14040010, 0x10000410, 0x14000410, 0x10040410, 0x14040410,
	0x10000014, 0x14000014, 0x10040014, 0x14040014, 0x10000414, 0x14000414, 0x10040414, 0x14040414,
	0x00100010, 0x04100010, 0x00140010, 0x04140010, 0x00100410, 0x04100410, 0x00140410, 0x04140410,
	0x00100014, 0x04100014, 0x00140014, 0x04140014, 0x00100414, 0x04100414, 0x00140414, 0x04140414,
	0x10100010, 0x14100010, 0x10140010, 0x14140010, 0x10100410, 0x14100410, 0x10140410, 0x14140410,
	0x10100014, 0x14100014, 0x10140014, 0x14140014, 0x10100414, 0x14100414, 0x10140414, 0x14140414,
	0x00001010, 0x04001010, 0x00041010, 0x04041010, 0x00001410, 0x04001410, 0x00041410, 0x04041410,
	0x00001014, 0x04001014, 0x00041014, 0x04041014, 0x00001414, 0x04001414, 0x00041414, 0x04041414,
	0x10001010, 0x14001010, 0x10041010, 0x14041010, 0x10001410, 0x14001410, 0x10041410, 0x14041410,
	0x10001014, 0x14001014, 0x10041014, 0x14041014, 0x10001414, 0x14001414, 0x10041414, 0x14041414,
	0x00101010, 0x04101010, 0x00141010, 0x04141010, 0x00101410, 0x04101410, 0x00141410, 0x04141410,
	0x00101014, 0x04101014, 0x00141014, 0x04141014, 0x00101414, 0x04101414, 0x00141414, 0x04141414,
	0x10101010, 0x14101010, 0x10141010, 0x14141010, 0x10101410, 0x14101410, 0x10141410, 0x14141410,
	0x10101014, 0x14101014, 0x10141014, 0x14141014, 0x10101414, 0x14101414, 0x10141414, 0x14141414,
};