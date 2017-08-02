#include "endian.h"

int16_t img_read_int16(struct img_io *io)
{
	int16_t v;
	io->read(&v, 2, io->uptr);
	return v;
}

int16_t img_read_int16_inv(struct img_io *io)
{
	int16_t v;
	io->read(&v, 2, io->uptr);
	return ((v >> 8) & 0xff) | (v << 8);
}

uint16_t img_read_uint16(struct img_io *io)
{
	uint16_t v;
	io->read(&v, 2, io->uptr);
	return v;
}

uint16_t img_read_uint16_inv(struct img_io *io)
{
	int16_t v;
	io->read(&v, 2, io->uptr);
	return (v >> 8) | (v << 8);
}


void img_write_int16(struct img_io *io, int16_t val)
{
	io->write(&val, 2, io->uptr);
}

void img_write_int16_inv(struct img_io *io, int16_t val)
{
	val = ((val >> 8) & 0xff) | (val << 8);
	io->write(&val, 2, io->uptr);
}

void img_write_uint16(struct img_io *io, uint16_t val)
{
	io->write(&val, 2, io->uptr);
}

void img_write_uint16_inv(struct img_io *io, uint16_t val)
{
	val = (val >> 8) | (val << 8);
	io->write(&val, 2, io->uptr);
}

uint32_t img_read_uint32(struct img_io *io)
{
	uint32_t v;
	io->read(&v, 4, io->uptr);
	return v;
}

uint32_t img_read_uint32_inv(struct img_io *io)
{
	uint32_t v;
	io->read(&v, 4, io->uptr);
	return (v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24);
}
