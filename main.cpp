#include <Windows.h>
#include <boost/make_shared.hpp>

#include "ui_showRGB.h"
#include "fileDriver.h"
#include "streamDecode.h"
#include "streamHandler.h"
#include "RGBWnd.h"

//[获取控制台窗口的句柄](https://blog.csdn.net/a199228/article/details/6696744)
typedef HWND (WINAPI *PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindowHwnd;

int main(int argc, char * argv[])
{
	if (argc < 2 )
	{
		return -1;
	}

	HMODULE hKernel32 = GetModuleHandle("kernel32");
	GetConsoleWindowHwnd = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

	boost::shared_ptr<CStreamDecode> decoder = boost::make_shared<CStreamDecode>();
	decoder->FFmpeg_H264DecoderInit();

	QApplication app(argc,argv);

	//QWidget *dialog = new QWidget;
	//boost::shared_ptr<Ui::Form> ui = boost::make_shared<Ui::Form>();
	//ui->setupUi(dialog);

	boost::shared_ptr<QGLCanvas> ui = boost::make_shared<QGLCanvas>();

	//boost::shared_ptr<RGBDataBuffer> data= boost::make_shared<RGBDataBuffer>(5, ui);

	boost::shared_ptr<CStreamHandler> handler = boost::make_shared<CStreamHandler>(decoder, ui);
	
	ui->show();

	CFileDriver h264File(argv[1], handler.get());

	//读h264文件，播放；文件读完之后，播放结束
	h264File.init();
	h264File.readStreamData();

	//h264文件作为视频源（不间断，文件结束后从头开始读取）
	//int ret;
	//do 
	//{
	//	if (!h264File.init())
	//	{
	//		std::cout << "h264File.init error" << std::endl;
	//		break;
	//	}

	//	ret = h264File.readStreamData();
	//	
	//} while (ret == AVERROR_EOF);

	//system("pause");
	//return 0;

	return app.exec();
	
}