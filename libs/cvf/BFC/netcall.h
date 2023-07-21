#pragma once

#include"BFC/def.h"
#include"BFC/bfstream.h"
#include"BFC/ctc.h"
#include<memory>
#include<map>
#include"opencv2/highgui.hpp"
#include"RudeSocket/socket.h"

_FF_BEG

class ObjStream
{
	std::shared_ptr<BMStream>  _stream=std::shared_ptr<BMStream>(new BMStream);

	static std::vector<int> _getArrayShape(BMStream &stream, int elemSize)
	{
		stream.Seek(0, SEEK_SET);

		const int MAX_DIM = 4;
		std::vector<int> shape;
		int n = 1;
		int totalSize = 0;
		int streamSize = stream.Size();
		for (int i = 0; i < MAX_DIM; ++i)
		{
			int m;
			stream >> m;
			shape.push_back(m);
			n *= m;
			totalSize = n*elemSize + shape.size() * sizeof(int);
			if ( uint(totalSize) >= stream.Size())
				break;
		}
		if (totalSize != stream.Size())
			throw std::exception("invalid data size");
		return shape;
	}
	template<typename _ValT, typename _AllocT>
	void getNDArray(_AllocT &alloc)
	{
		std::vector<int> shape = this->_getArrayShape(*_stream, sizeof(_ValT));
		void *data = alloc(shape);
		int size = 1;
		for (auto &m : shape)
			size *= m;
		_stream->Read(data, size * sizeof(_ValT), 1);
	}
public:
	ObjStream()
	{}
	template<typename _ValT>
	ObjStream(const _ValT &obj)
	{
		(*this) << obj;
	}
	ObjStream(const char *str)
	{
		(*this) << std::string(str);
	}
	static ObjStream fromImage(const cv::Mat &img, const std::string &ext=".jpg")
	{
		std::vector<uchar> buf;
		if (!cv::imencode(ext, img, buf))
			throw std::exception("imencode failed");

		ObjStream ob;
		ob.stream().Write(&buf[0],buf.size(),1);
		return ob;
	}
	BMStream& stream() const
	{
		return *_stream;
	}
	template<typename _ValT>
	_ValT get()
	{
		_ValT obj;

		_stream->Seek(0, SEEK_SET);
		(*this) >> obj;
		return obj;
	}
	
	template<typename _ChannelT>
	Mat  getMat(int channels=1)
	{
		std::vector<int> shape = this->_getArrayShape(*_stream, sizeof(_ChannelT));

		int nelems = 1;
		for (auto v : shape)
			nelems *= v;
		if (nelems <= 0)
			return Mat();

		while (!shape.empty() && shape.back() == 1)
			shape.pop_back();

		int dims = (int)shape.size();
		Mat m;
		if (dims <= 2 || (dims == 3 && shape[2] <= CV_CN_MAX))
		{
			int rows = dims >= 1 ? shape[0] : 1, cols = dims >= 2 ? shape[1] : 1, channels = dims >= 3 ? shape[2] : 1;
			int type = CV_MAKETYPE(cv::DataType<_ChannelT>::depth, channels);

			m.create(rows, cols, type);
			CV_Assert(m.elemSize()*m.cols == m.step);
		}
		else
		{
			int type = CV_MAKETYPE(cv::DataType<_ChannelT>::depth, 1);
			m.create(dims, &shape[0], type);
		}

		_stream->Read(m.data, sizeof(_ChannelT)*nelems, 1);
		return m;
	}
	template<typename _Tp>
	static _Tp _getChannelT(cv::Point_<_Tp>);

	template<typename _Tp>
	static _Tp _getChannelT(cv::Point3_<_Tp>);

	template<typename _Tp, int n>
	static _Tp _getChannelT(cv::Vec<_Tp,n>);

	template<typename _Tp>
	static _Tp _getChannelT(_Tp);

	template<typename _ValT>
	std::vector<_ValT>  getMatAsVector()
	{
		Mat m = this->getMat<decltype(_getChannelT(_ValT()))>();
		
		CV_Assert(m.step == m.elemSize()*m.cols);
		int size = m.elemSize()*m.rows*m.cols;
		CV_Assert(size % sizeof(_ValT) == 0);
		size /= sizeof(_ValT);
		
		std::vector<_ValT>  v(size);
		if (size > 0)
			memcpy(&v[0], m.data, size * sizeof(_ValT));
		return v;
	}

	Mat getImage()
	{
		int size = _stream->Size();
		std::vector<uchar> buf(size);
		_stream->Read(&buf[0], size, 1);
		return cv::imdecode(buf, cv::IMREAD_UNCHANGED);
	}

	int size() const
	{
		return _stream->Size();
	}
	const char* data() const
	{
		return _stream->Buffer();
	}
	/*void setBytes(const void *bytes, int size)
	{
		_stream->Clear();
		_stream->Write(bytes, size, 1);
	}*/

	template<typename _Stream>
	friend void BFSWrite(_Stream &os, const ObjStream &obj)
	{
		os << (int32)obj.size();
		os.Write(obj.data(), obj.size(), 1);
	}

	template<typename _Stream>
	friend void BFSRead(_Stream &is, ObjStream &obj)
	{
		int32 dsize;
		is >> dsize;
		auto stream = std::shared_ptr<BMStream>(new BMStream);
		stream->Clear();
		stream->Resize(dsize);
		is.Read((char*)stream->Buffer(), dsize, 1);
		obj._stream = stream;
	}

	//return short==types of fixed size
	static short is_supported_type(char);
	static short is_supported_type(uchar);
	static short is_supported_type(int);
	static short is_supported_type(uint);
	static short is_supported_type(float);
	static short is_supported_type(double);
	template<typename _ValT>
	static short is_supported_type(cv::Point_<_ValT>);
	//return int ==types of unknown encoded size
	/*static int is_supported_type(std::string);
	template<typename _ValT>
	static int is_supported_type(std::vector<_ValT>);*/
	//return char == unsurpported types
	static char is_supported_type(...);

	template<typename _ValT>
	friend ObjStream& operator<<(ObjStream &os, const _ValT &val)
	{
		static_assert(sizeof(is_supported_type(_ValT()))>1, "unsurpported type");
		//(*os._stream) << val;
		(*os._stream).Write(&val, sizeof(val), 1);
		return os;
	}
	template<typename _ValT>
	friend ObjStream& operator>>(ObjStream &is, _ValT &val)
	{
		static_assert(sizeof(is_supported_type(_ValT())) > 1, "unsurpported type");
		//(*is._stream) >> val;
		(*is._stream).Read(&val, sizeof(val), 1);
		return is;
	}

	friend ObjStream& operator<<(ObjStream &os, const std::string &val)
	{
		os.stream().Write(val.data(), val.size(), 1);
		return os;
	}
	friend ObjStream& operator>>(ObjStream &os, std::string &val)
	{
		int size = os.size();
		val.resize(size);
		if(size>0)
			os.stream().Read(&val[0], val.size(), 1);
		return os;
	}
	template<typename _ValT>
	friend ObjStream& operator << (ObjStream &os, const std::vector<_ValT> &v)
	{
		bool withElemSize = sizeof(is_supported_type(_ValT()))!=sizeof(short);
		os << (int32)v.size();
		if (withElemSize)
		{
			for (auto &x : v)
			{
				long p = os._stream->Tell();
				os << (int32)0 << x;
				
				int32 xsize = os._stream->Tell() - p - sizeof(int32);
				os.stream().WriteAt(&xsize, p, sizeof(int32), 1);
			}
		}
		else
		{
			if(!v.empty())
				os._stream->Write(&v[0], sizeof(_ValT), v.size());
		}
		return os;
	}
	template<typename _ValT>
	friend ObjStream& operator >> (ObjStream &is, std::vector<_ValT> &v)
	{
		bool withElemSize = sizeof(is_supported_type(_ValT())) != sizeof(short);
		int32 size;
		is >> size;
		v.resize(size);

		if (withElemSize)
		{
			char *buf = (char*)is.stream().Buffer();
			long p = sizeof(int32);
			long dsize = is._stream->Size();

			ObjStream ts;
			for (int i = 0; i < v.size(); ++i)
			{
				is._stream->ReadAt(&size, p, sizeof(int32), 1);
				p += sizeof(int32);
				ts._stream->SetBuffer(buf + p, size, false);
				ts >> v[i];
				p += size;
			}
		}
		else
		{
			FFAssert(is._stream->Size() >= sizeof(_ValT)*size);
			is._stream->Read(&v[0], sizeof(_ValT), size);
		}
		return is;
	}

	template<typename _ValT, int _N>
	friend ObjStream& operator<<(ObjStream &os, const _ValT(&arr)[_N])
	{
		(*os._stream) << _N << arr;
		return os;
	}
	template<typename _ValT, int _N>
	friend ObjStream& operator >> (ObjStream &is, _ValT(&arr)[_N])
	{
		is.getNDArray<_ValT>([&arr](const std::vector<int> &shape) {
			FFAssert(shape[0] == _N);
			return arr;
		});
		return is;
	}
	template<typename _ValT, int _M, int _N>
	friend ObjStream& operator<<(ObjStream &os, const _ValT(&arr)[_M][_N])
	{
		(*os._stream) << _M << _N << arr;
		return os;
	}
	template<typename _ValT, int _M, int _N>
	friend ObjStream& operator>>(ObjStream &is, const _ValT(&arr)[_M][_N])
	{
		is.getNDArray<_ValT>([&arr](const std::vector<int> &shape) {
			FFAssert(shape[0] == _M && shape[1]==_N);
			return arr;
		});
		return is;
	}
	friend ObjStream& operator<<(ObjStream &os, const cv::Mat &m)
	{
		os << m.rows << m.cols << m.channels();
		for (int i = 0; i < m.rows; ++i)
			os.stream().Write(m.ptr(i), m.elemSize()*m.cols, 1);
		return os;
	}
	template<typename _PixelT>
	friend ObjStream& operator << (ObjStream &os, const cv::Mat_<_PixelT> &m)
	{
		os << (const cv::Mat&)m;
		return os;
	}
	template<typename _PixelT>
	friend ObjStream& operator >> (ObjStream &is, cv::Mat_<_PixelT> &m)
	{
		typedef typename cv::Mat_<_PixelT>::channel_type _ValT;
		Mat t = is.getMat<_ValT>();
	//	FFAssert(t.dims >= 3 && m.channels() == 1 || t.dims == 2 && (m.channels() == t.channels()||m.channels()==1));
		m = t;
		return is;
	}
	template<typename _ValT,int m,int n>
	friend ObjStream& operator<<(ObjStream &stream, const cv::Matx<_ValT, m, n> &v)
	{
		stream.stream() << m << n;
		stream.stream().Write(v.val, sizeof(_ValT)*m*n, 1);
		return stream;
	}
	template<typename _ValT, int m, int n>
	friend ObjStream& operator>>(ObjStream &stream, cv::Matx<_ValT, m, n> &v)
	{
		stream.getNDArray<_ValT>([&v](const std::vector<int> &shape) {
			FFAssert(shape.size()>=2 && shape[0] == m && shape[1] == n);
			return v.val;
		});
		return stream;
	}
};

//typedef std::map<std::string, ObjStream> NetObjs;

class NetObjs
	:public std::map<std::string, ObjStream>
{
	typedef std::map<std::string, ObjStream> _BaseT;
public:
	using _BaseT::_BaseT;

	//operator bool() 
	bool hasError()
	{
		auto itr = this->find("error");// != this->end()
		if (itr == this->end())
			return false;
		return itr->second.get<int>() < 0;
	}
	bool operator!()
	{
		return !hasError();
	}
};

inline std::string netcall_encode(const NetObjs &objs)
{
	OBMStream os;
	os << int32(0);

	for (auto &v : objs)
	{
		os << v.first << v.second;
	}
	int32 totalSize = os.Size() - sizeof(int32);
	os.Seek(0, SEEK_SET);
	os << totalSize;

	const char *data = os.Buffer();
	return std::string(data, data + os.Size());
}

inline NetObjs netcall_decode(const std::string &data, bool skipHead=true)
{
	IBMStream is;
	is.SetBuffer((char*)data.data(), data.size(), false);

	if(skipHead)
		is.Seek(sizeof(int32), SEEK_SET);

	NetObjs objs;

	std::string name;
	ObjStream obj;
	while (is.Tell() < is.Size())
	{
		is >> name >> obj;
		objs.emplace(name, obj);
	}
	return objs;
}

inline std::string netcall_recv(rude::Socket *net)
{
	auto readData = [net](void *buf, int size) {
		int r = net->read((char*)buf, size);
		if (r != size)
			FF_EXCEPTION1("net read failed");
		return r;
	};

	int32 size;
	readData(&size, sizeof(size));

	std::string data;
	data.resize(sizeof(size) + size);
	readData(&data[sizeof(size)], size);
	memcpy(&data[0], &size, sizeof(size));

	return data;
}

class _BFC_API NetcallServer
	:public rude::Socket
{
public:
	NetcallServer(){}

	NetcallServer(const char *server, int port)
	{
		this->connect(server, port);
	}

	void connect(const char *server, int port)
	{
		if (!this->rude::Socket::connect(server, port))
			FF_EXCEPTION1("failed to connect with netcall server");
	}

	NetObjs  call(const NetObjs &objs, bool recv=true)
	{
		auto data = netcall_encode(objs);
		int r=this->send(data.data(), data.size());
		if (r != data.size())
			FF_EXCEPTION1(ff::StrFormat("netcall send failed (return %d, expected %d)", r, (int)data.size()));
		
		if (!recv)
			return NetObjs();

		auto rdata = netcall_recv(this);
		return netcall_decode(rdata);
	}
	void sendExit()
	{
		ff::NetObjs objs = {
			{ "cmd","exit" }
		};
		this->call(objs, false);
	}
};


_FF_END


