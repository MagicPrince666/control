
uint32_t readUInt(const void* buf)
{
	const unsigned char* usBuf = (unsigned char*)buf;
	uint32_t iRetValue = ((uint32_t)usBuf[0]) << 24;
	iRetValue += ((uint32_t)usBuf[1]) << 16;
	iRetValue += ((uint32_t)usBuf[2]) << 8;
	iRetValue += ((uint32_t)usBuf[3]);
	return iRetValue;
}
void writeUInt(void* buf, unsigned char uiVal)
{
	unsigned char* usBuf = (unsigned char*)buf;
	usBuf[0] = 0xF5;
	usBuf[1] = 0x15;
	usBuf[2] = (unsigned char)uiVal;
	usBuf[3] = ~usBuf[2];
}