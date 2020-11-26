/*
 Lightweight data stream class.
*/

#ifndef __LDS_H__
#define __LDS_H__

#include <string>
#include <assert.h>
#include "FFDef.h"

class LrpcDataStream
{
public:
	enum LRPC_ACCESS_MODE
	{
		MODE_READ,
		MODE_WRITE
	};

	LrpcDataStream(char* data, size_t len, LRPC_ACCESS_MODE mode, bool auto_free_buffer = false)
		: m_mode(mode), m_auto_free_buffer(auto_free_buffer)
	{
		if (mode == MODE_READ) {
			if (data == NULL || len == 0) {
				// TODO: throw exception
				// data buffer must not be null in parsing mode 
			}
			m_buf_ptr = data;
			m_buf_len = len;
		} else if (mode == MODE_WRITE) {
			if (data != NULL) {
				// external buffer is not supported
				assert(0);
			}
			// allocate buffer
			m_buf_len = (len == 0) ? DEFAULT_BUF_LEN : len;
			m_buf_ptr = (char*)malloc(m_buf_len);
		}
		m_offset = 0;
	}
	
	~LrpcDataStream(void)
	{
		if (m_auto_free_buffer && m_buf_ptr != NULL) {
			free(m_buf_ptr);
		}
	}

	// reuse write stream buffer
	void reset()
	{
		if (m_mode == MODE_READ) {
			assert(0);
			return;
		}
		memset(m_buf_ptr, 0, m_buf_len);
		m_offset = 0;
	}

	bool seekto(unsigned int offset)
	{
		if (offset >= m_buf_len) {
			assert(0);
			return false;
		}
		m_offset = offset;
		return true;
	}

	void release()
	{
		if (m_buf_ptr != NULL)
			free(m_buf_ptr);
	}
	
	void resize(unsigned int new_size)
	{
		if (m_mode != LrpcDataStream::MODE_WRITE) {
			// TODO:
			// throw exception
			assert(FALSE);
		}

		if (new_size < m_buf_len) {
			return;
		}
		
		m_buf_ptr = (char*)realloc(m_buf_ptr, new_size);
		m_buf_len = new_size;
	}

	void read(char* databuf, int len)
	{
		assert(m_mode == LrpcDataStream::MODE_READ);
		assert((m_offset + len) <= m_buf_len);
		memcpy(databuf, m_buf_ptr+m_offset, len);
		m_offset += len;
	}

	void write(const char* data, int len)
	{
		assert(m_mode == LrpcDataStream::MODE_WRITE);
		if (len == 0)
			return;

		perpare_buffer(len);
		assert(data != NULL);
		memcpy(m_buf_ptr + m_offset, data, len);
		m_offset += len;
	}

	// Reading methods
	LrpcDataStream& operator>>(bool& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(bool)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(bool));
		m_offset += sizeof(bool);
		return *this; 
	}

	LrpcDataStream& operator>>(int8& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(int8)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(int8));
		m_offset += sizeof(int8);
		return *this;
	}

	LrpcDataStream& operator>>(uint8& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(uint8)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(uint8));
		m_offset += sizeof(uint8);
		return *this;
	}

	LrpcDataStream& operator>>(int16& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(int16)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(int16));
		m_offset += sizeof(int16);
		return *this; 
	}

	LrpcDataStream& operator>>(uint16& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(uint16)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(uint16));
		m_offset += sizeof(uint16);
		return *this; 
	}

	LrpcDataStream& operator>>(int32& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(int32)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(int32));
		m_offset += sizeof(int32);
		return *this; 
	}

	LrpcDataStream& operator>>(uint32& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(uint32)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(uint32));
		m_offset += sizeof(uint32);
		return *this; 
	}

	LrpcDataStream& operator>>(int64& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(int64)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(int64));
		m_offset += sizeof(int64);
		return *this;
	}

	LrpcDataStream& operator>>(uint64& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(uint64)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(uint64));
		m_offset += sizeof(uint64);
		return *this; 
	}

	LrpcDataStream& operator>>(float& val)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(float)) <= m_buf_len);

		memcpy(&val, m_buf_ptr+m_offset, sizeof(float));
		m_offset += sizeof(float);
		return *this; 
	}

	LrpcDataStream& operator>>(double& dbl)
	{
		assert(m_mode == MODE_READ);
		assert((m_offset + sizeof(double)) <= m_buf_len);

		memcpy(&dbl, m_buf_ptr+m_offset, sizeof(double));
		m_offset += sizeof(double);
		return *this; 
	}

	LrpcDataStream& operator>>(std::string& str)
	{
		assert(m_mode == MODE_READ);
		str = m_buf_ptr + m_offset;
		assert((m_offset + str.length() + 1) <= m_buf_len);
		m_offset += str.length() + 1;
		return *this;
	}

	// Writing methods
	LrpcDataStream& operator<<(bool& val)
	{
		perpare_buffer(sizeof(bool));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(bool));
		m_offset += sizeof(bool);
		return *this;
	}

	LrpcDataStream& operator<<(int8& val)
	{
		perpare_buffer(sizeof(int8));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(int8));
		m_offset += sizeof(int8);
		return *this;
	}

	LrpcDataStream& operator<<(uint8& val)
	{
		perpare_buffer(sizeof(uint8));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(uint8));
		m_offset += sizeof(uint8);
		return *this;
	}

	LrpcDataStream& operator<<(int16& val)
	{
		perpare_buffer(sizeof(int16));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(int16));
		m_offset += sizeof(int16);
		return *this;
	}
	
	LrpcDataStream& operator<<(uint16& val)
	{
		perpare_buffer(sizeof(uint16));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(uint16));
		m_offset += sizeof(uint16);
		return *this;
	}

	LrpcDataStream& operator<<(int32& val)
	{
		perpare_buffer(sizeof(int32));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(int32));
		m_offset += sizeof(int32);
		return *this;
	}

	LrpcDataStream& operator<<(uint32& val)
	{
		perpare_buffer(sizeof(uint32));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(uint32));
		m_offset += sizeof(uint32);
		return *this;
	}

	LrpcDataStream& operator<<(int64& val)
	{
		perpare_buffer(sizeof(int64));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(int64));
		m_offset += sizeof(int64);
		return *this;
	}

	LrpcDataStream& operator<<(uint64& val)
	{
		perpare_buffer(sizeof(uint64));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(uint64));
		m_offset += sizeof(uint64);
		return *this;
	}

	LrpcDataStream& operator<<(float& val)
	{
		perpare_buffer(sizeof(float));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(float));
		m_offset += sizeof(float);
		return *this;
	}

	LrpcDataStream& operator<<(double& val)
	{
		perpare_buffer(sizeof(double));
		memcpy(m_buf_ptr + m_offset, &val, sizeof(double));
		m_offset += sizeof(double);
		return *this;
	}

	LrpcDataStream& operator<<(const std::string& str)
	{
		*this << str.c_str();
		return *this;
	}

	LrpcDataStream& operator<<(const char* str)
	{
		int len = strlen(str) + 1;
		perpare_buffer(len);
#if defined(WIN32) || defined(WINCE)
		strcpy_s(m_buf_ptr + m_offset, len, str);
#else
		strcpy(m_buf_ptr + m_offset, str.c_str());
#endif
		m_offset += len;
		return *this;
	}
	
	char* get_buffer() { return m_buf_ptr; }
	char* get_cur_ptr() { return m_buf_ptr + m_offset; }
	size_t get_datalen() { return m_offset; }
	bool read_done() { return (m_offset == m_buf_len); }

private:
	void perpare_buffer(size_t add_len)
	{
		assert(m_mode == MODE_WRITE);
		if ((m_offset + add_len) > m_buf_len) {
			// TODO: optimize for small size data...
			size_t new_len = ((m_offset + add_len) / 2048 + 1) * 4096;
			m_buf_ptr = (char*)realloc(m_buf_ptr, new_len);
			m_buf_len = new_len;
		//	printf("\t*** realloc, size = %d *** \n", new_len);
		}
	}

	char* m_buf_ptr;
	size_t m_buf_len;
	size_t m_offset;
	LRPC_ACCESS_MODE m_mode;
	bool m_auto_free_buffer;
	static const unsigned int DEFAULT_BUF_LEN = 1024;
	
};

#endif
