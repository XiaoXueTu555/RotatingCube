#include <iostream>
#include <string>
#include <cmath>
#include <initializer_list>
#include <algorithm> //std::copy_n

/*用于实现延时函数*/
#include <chrono>
#include <thread>

//四舍五入函数
inline int Round(double num)
{
	return static_cast<int>(std::round(num));
}

class Camera;

// Camera的属性
class Camera_properties
{
	/*将Camera设置成友元类，允许Camera类访问init和clear函数*/
	friend Camera;

	//这样比较方便
	using __Camera = Camera_properties;
public:
	inline static int get_width()
	{
		return __Camera::width;
	}
	inline static int get_height()
	{
		return __Camera::height;
	}
	/*x-y平面到照相机平面的偏量*/
	inline static int get_Z_axis_deviation()
	{
		return __Camera::Z_axis_deviation;
	}
protected:
	inline static void init(int screen_width, int screen_height, int screen_z_axis_deviation)
	{
		__Camera::width = screen_width;
		__Camera::height = screen_height;
		__Camera::Z_axis_deviation = screen_z_axis_deviation;
		delete[] __Camera::camera_plane;
		__Camera::camera_plane = new char[__Camera::width * __Camera::height];
		__Camera::clear('\040');
	}
	inline static void clear(char pixel)
	{
		for (int i = 0; i < __Camera::width * __Camera::height; i++)
		{
			__Camera::camera_plane[i] = pixel;
		}
	}
private:
	static int width;
	static int height;
	static int Z_axis_deviation;
	static char* camera_plane;
};
/*初始化照相机的属性*/
int Camera_properties::width = 40;					//照相机平面的（屏幕）宽度
int Camera_properties::height = 40;					//照相机平面的（屏幕）高度
int Camera_properties::Z_axis_deviation = -100;		//照相机平面距离x-y平面的偏移
char* Camera_properties::camera_plane = nullptr;	//照相机平面

//笛卡尔坐标系
class vector3d
{
public:
	vector3d() : x(0.0f), y(0.0f), z(0.0f) {}
	vector3d(double x, double y, double z = 0)
		:x(x), y(y), z(z) {}
	vector3d(std::initializer_list<double> list)
	{
		*this = list;
	}

	vector3d& operator=(std::initializer_list<double> list)
	{
		if (list.size() == 2)
		{
			x = *list.begin();
			y = *(list.begin() + 1);
			z = 0.0f;
		}
		else if (list.size() == 3)
		{
			x = *list.begin();
			y = *(list.begin() + 1);
			z = *(list.begin() + 2);
		}
		else
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}
		return *this;
	}
	vector3d operator+(const vector3d& v) const
	{
		vector3d result =
		{ 
			this->x + v.x, 
			this->y + v.y, 
			this->z + v.z 
		};
		return result;
	}
	vector3d operator-(const vector3d& v) const
	{
		vector3d result =
		{ 
			this->x - v.x, 
			this->y - v.y, 
			this->z - v.z 
		};
		return result;
	}
	vector3d operator*(const double coe) const
	{
		vector3d result =
		{
			x * coe,
			y * coe,
			z * coe
		};
		return result;
	}
	vector3d& operator*=(const double coe)
	{
		return *this = (*this * coe);
	}

	/*向量的模长*/
	double magnitude() const
	{
		return sqrt(x * x + y * y + z * z);
	}

	//两个向量的向量积
	vector3d vector_product(const vector3d& v) const
	{
		vector3d result;
		result.x = this->y * v.z - this->z * v.y;
		result.y = this->z * v.x - this->x * v.z;
		result.z = this->x * v.y - this->y * v.x;
		return result;
	}

	/*投影到平面坐标系*/
	vector3d Project_onto_plane_xy() const
	{
		vector3d result = { x, y, 0 };
		return result;
	}
	vector3d Project_onto_plane_xz() const
	{
		vector3d result = { x, 0, z };
		return result;
	}
	vector3d Project_onto_plane_yz() const
	{
		vector3d result = { 0, y, z };
		return result;
	}

	vector3d& rotate_x(const double rad)
	{
		double y1 = y * cos(rad) - z * sin(rad);
		double z1 = y * sin(rad) + z * cos(rad);
		y = y1;
		z = z1;
		return *this;
	}
	vector3d& rotate_y(const double rad)
	{
		double x1 = x * cos(rad) + z * sin(rad);
		double z1 = -x * sin(rad) + z * cos(rad);
		x = x1;
		z = z1;
		return *this;
	}
	vector3d& rotate_z(const double rad)
	{
		double x1 = x * cos(rad) - y * sin(rad);
		double y1 = x * sin(rad) + y * cos(rad);
		x = x1;
		y = y1;
		return *this;
	}
public:
	double x;
	double y;
	double z;
};

using Point = vector3d;

//笛卡尔坐标系
class Triangle
{
public:
	Triangle() {}
	Triangle(const Point& A, const Point& B, const Point& C)
	{
		this->A = A;
		this->B = B;
		this->C = C;
	}
	Triangle(std::initializer_list<Point> list)
	{
		if (list.size() == 3)
		{
			A = *list.begin();
			B = *(list.begin() + 1);
			C = *(list.begin() + 2);
		}
	}
public:
	Point A, B, C;
};

//面
class Face
{
public:
	Face() {}
	//ABC构成三角形，BCD构成三角形
	Face(Point A, Point B, Point C, Point D)
	{
		this->A = A;
		this->B = B;
		this->C = C;
		this->D = D;
	}
	Face(std::initializer_list<Point> list)
	{
		if (list.size() == 4)
		{
			A = *list.begin();
			B = *(list.begin() + 1);
			C = *(list.begin() + 2);
			D = *(list.begin() + 3);
		}
	}

public:
	//平面的中心点与照相机的距离
	double Distance() const
	{
		//BC中点坐标的z偏量到照相机所在平面的距离
		return (B.z + C.z) / 2 - (Camera_properties::get_Z_axis_deviation());
	}
public:
	Point A, B, C, D;
};

/*由6个三角形组成的立方体*/
class Cube
{
public:
	Cube(const double side = 20, const char* surface_pixel = nullptr)
	{
		/*定义该立方体的六个面，每个面由四个顶点构成，第1，2，3个顶点构成一个三角形，
		并且第2，3，4也构成一个三角形，同时，这两个三角形拼在一起构成一个正方形*/
		p[0] = { {-1, 1, -1}, {-1, -1, -1}, {1, 1, -1}, {1, -1, -1} };
		p[1] = { {1, 1, -1}, {1, -1, -1}, {1, 1, 1}, {1, -1, 1} };
		p[2] = { {1, -1, -1}, {-1, -1, -1}, {1, -1, 1}, {-1, -1, 1} };
		p[3] = { {-1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}, {-1, 1, 1} };
		p[4] = { {-1, 1, -1}, {1, 1, -1}, {-1, 1, 1}, {1, 1, 1} };
		p[5] = { {-1, 1, 1}, {-1, -1, 1}, {1, 1, 1}, {1, -1, 1} };

		/*设置立方体各个面的pixel*/
		if (surface_pixel == nullptr)
		{
			this->surface_pixel[0] = '+';
			this->surface_pixel[1] = '&';
			this->surface_pixel[2] = '*';
			this->surface_pixel[3] = '#';
			this->surface_pixel[4] = '@';
			this->surface_pixel[5] = '^';
		}	
		else
		{
			for (int i = 0; i < 6; i++)
				this->surface_pixel[i] = surface_pixel[i];
		}

		//缩放这个三角形
		for (int i = 0; i < 6; i++)
		{
			p[i].A *= side / 2.0;
			p[i].B *= side / 2.0;
			p[i].C *= side / 2.0;
			p[i].D *= side / 2.0;
		}
	}

	void rotate_x(const double rad)
	{
		for (int i = 0; i < 6; i++)
		{
			p[i].A.rotate_x(rad);
			p[i].B.rotate_x(rad);
			p[i].C.rotate_x(rad);
			p[i].D.rotate_x(rad);
		}
	}
	void rotate_y(const double rad)
	{
		for (int i = 0; i < 6; i++)
		{
			p[i].A.rotate_y(rad);
			p[i].B.rotate_y(rad);
			p[i].C.rotate_y(rad);
			p[i].D.rotate_y(rad);
		}
	}
	void rotate_z(const double rad)
	{
		for (int i = 0; i < 6; i++)
		{
			p[i].A.rotate_z(rad);
			p[i].B.rotate_z(rad);
			p[i].C.rotate_z(rad);
			p[i].D.rotate_z(rad);
		}
	}

public:
	double get_cube_side() const
	{
		return (this->p[0].A - this->p[0].B).magnitude();
	}
public:
	Face p[6];
	char surface_pixel[6];
};

//照相机（屏幕）
/*该照相机所在平面与x-y平面平行，并且在其下方。
（突然发现，这个类里面我写的全是静态的东西，想了一下，我主要是想让照相机这个对象
在全局中有且仅有一个，所以这样设计。）*/
class Camera
{
public:
	static void init(
		int screen_width = Camera_properties::width,
		int screen_height = Camera_properties::height, 
		int screen_z_axis_deviation = Camera_properties::Z_axis_deviation)
	{
		Camera_properties::init(screen_width, screen_height, screen_z_axis_deviation);
	}		

	static void clear()
	{
		Camera_properties::clear('\040');
	}

	/*更新函数，输出照相机捕获的画面*/
	static void update()
	{
		// 输出 buffer 内容，每行输出 10 个字符
		int length = Camera::width * Camera::height;
		for (int i = 0; i < length; i += Camera::width) {
			fwrite(Camera::camera_plane + i, sizeof(char), Camera::width, stdout);
			putchar('\n');
		}
	}

public:
	/*采用屏幕坐标系，非直角坐标系*/
	static void SetText(int x, int y, std::string text)
	{
		/*snprintf函数会在结尾自动填写一个'\0'，这会导致在输出字符数组时产生吞字符的问题，
		debug好久才找到这个bug*/
		//snprintf(Camera::camera_plane + (y * Camera::width + x), (Camera::width * Camera::height) - (y * Camera::width + x), text.c_str());

		std::copy_n(text.c_str(), text.size(), Camera::camera_plane + (y * Camera::width + x));
	}
public:
	/*采用直角坐标系，非屏幕坐标系*/

	//画点函数，平面直角坐标系（非屏幕坐标系），也可以理解为摄像机的视口坐标系
	static void SetPixel(double x, double y, char pixel)
	{
		//屏幕坐标点
		int x1 = width / 2 + Round(x);
		int y1 = height / 2 - Round(y);
		if (x1 < 0 || y1 < 0 
			|| x1 >= Camera::width || y1 >= Camera::height)
		{
			return;
		}
		Camera::write_plane_val(x1, y1, pixel);
	}

	static void SetPixel(int x, int y, char pixel)
	{
		//屏幕坐标点
		int x1 = width / 2 + x;
		int y1 = height / 2 - y;
		if (x1 < 0 || y1 < 0 
			|| x1 >= Camera::width || y1 >= Camera::height)
		{
			return;
		}
		Camera::write_plane_val(x1, y1, pixel);
	}

	static void SetPixel(const Point& p, char pixel)
	{
		Camera::SetPixel(p.x, p.y, pixel);
	}

	/*渲染一个三角形*/
	static void SetTriangle(const Triangle& tr, char pixel)
	{
		/*只能绘制xy平面上的三角形，这要求z=0*/

		//计算向量
		const vector3d AB = tr.B - tr.A;
		const vector3d BC = tr.C - tr.B;

		//若该三角形的三个点是共线的
		if (AB.vector_product(BC).z == 0)
		{
			return;
		}

		/*找到包裹三角形的矩形*/
		const int left	= Round(std::min(std::min(tr.A.x, tr.B.x), tr.C.x));
		const int right	= Round(std::max(std::max(tr.A.x, tr.B.x), tr.C.x));
		const int top	= Round(std::max(std::max(tr.A.y, tr.B.y), tr.C.y));
		const int down	= Round(std::min(std::min(tr.A.y, tr.B.y), tr.C.y));

		Camera::SetPixel(tr.A, pixel);
		Camera::SetPixel(tr.B, pixel);
		Camera::SetPixel(tr.C, pixel);

		for (int y = down; y <= top; y++)
		{
			for (int x = left; x <= right; x++)
			{
				Point P = { (double)x, (double)y, 0 };
				vector3d PA = tr.A - P;
				vector3d PB = tr.B - P;
				vector3d PC = tr.C - P;
				//使用向量乘积法判断点(x,y)是否在三角形内部
				double Cross_PAB = PA.vector_product(PB).z;
				double Cross_PBC = PB.vector_product(PC).z;
				double Cross_PCA = PC.vector_product(PA).z;
				if ((Cross_PAB > 0 && Cross_PBC > 0 && Cross_PCA > 0)
					||
					(Cross_PAB < 0 && Cross_PBC < 0 && Cross_PCA < 0))
				{
					Camera::SetPixel(x, y, pixel);
				}
				else if(Cross_PAB == 0 || Cross_PBC == 0 || Cross_PCA == 0)
				{
					Camera::SetPixel(x, y, pixel);
				}
			}
		}
	}

	/*绘制x-y坐标系平面*/
	static void Set_plane_xy()
	{
		for (int i = -(Camera::width / 2); i < Camera::width / 2; i++)
		{
			Camera::SetPixel(i, 0, '-');
		}
		for (int i = -(Camera::height / 2); i < Camera::height / 2; i++)
		{
			Camera::SetPixel(0, i, '|');
		}
		Camera::SetPixel(0, 0, '+');
		Camera::SetPixel(Camera::width / 2 - 1, 0, '>');
		Camera::SetPixel(Camera::width / 2 - 1, -1, 'X');
		Camera::SetPixel(0, Camera::height / 2 - 1, '^');
		Camera::SetPixel(-1, Camera::height / 2 - 1, 'Y');
	}

	/*渲染一个面*/
	static void SetFace(const Face& face, char pixel)
	{
		/*由于Face面的四个点组成这样一个关系：前三个点构成一个三角形，后三个点构成一个三角形，
		所以在投影到xy平面时，第二个和第三个点是共用的，于是单独拿出来处理，这样可以保证这四个点
		仅仅投影一遍。*/
		Point temp_B = face.B.Project_onto_plane_xy();
		Point temp_C = face.C.Project_onto_plane_xy();

		Triangle tr1 = { face.A.Project_onto_plane_xy(), temp_B, temp_C };
		Triangle tr2 = { temp_B, temp_C, face.D.Project_onto_plane_xy() };

		Camera::SetTriangle(tr1, pixel);
		Camera::SetTriangle(tr2, pixel);
	}

	/*渲染一个立方体*/
	static void Setcube(const Cube& cube)
	{
		/*由于一次性最多只能观测到立方体的三个面，所以计算距离摄像机最近的三个面进行渲染。
		而一旦能观察到某一个面的时候，这个面是一定无遮挡的，所以只需要对着三个面正常进行渲染即可*/
		double d[6] = {};
		int indices[6] = {};
		for (int i = 0; i < 6; i++)
		{
			d[i] = cube.p[i].Distance();
			indices[i] = i;
		}
		/*对d数据从大到小排序 冒泡排序*/
		for (int i = 0; i < 5; i++) 
		{
			for (int j = 0; j < 5 - i; j++) 
			{
				if (d[j] > d[j + 1])
				{
					// 交换值
					double tempValue = d[j];
					d[j] = d[j + 1];
					d[j + 1] = tempValue;

					// 交换下标
					int tempIndex = indices[j];
					indices[j] = indices[j + 1];
					indices[j + 1] = tempIndex;
				}
			}
		}

		SetText(0, 0, "摄像机正在观测的面：" + std::to_string(indices[0]) + ", " + std::to_string(indices[1]) + ", " + std::to_string(indices[2]));

		/*从远到近渲染面*/
		for (int i = 2; i >= 0; i--)
		{
			Camera::SetFace(cube.p[indices[i]], cube.surface_pixel[indices[i]]);
		}
	}

public:
	static int get_width()
	{
		return Camera_properties::width;
	}

	static int get_height()
	{
		return Camera_properties::height;
	}

	/*x-y平面到照相机平面的偏量*/
	static int get_Z_axis_deviation()
	{
		return Camera_properties::Z_axis_deviation;
	}

private:
	/*得到照相机平面上的点，屏幕坐标系*/
	inline static char read_plane_val(int x, int y)
	{
		return Camera::camera_plane[y * Camera::width + x];
	}

	/*写入照相机平面上的点，屏幕坐标系*/
	inline static void write_plane_val(int x, int y, char pixel)
	{
		Camera::camera_plane[y * Camera::width + x] = pixel;
	}

private:
	static const int& width;
	static const int& height;
	static const int& Z_axis_deviation;
	static char* &camera_plane;
};
/*初始化内部成员*/
const int& Camera::width = Camera_properties::width;
const int& Camera::height = Camera_properties::height;
const int& Camera::Z_axis_deviation = Camera_properties::Z_axis_deviation;
char* &Camera::camera_plane = Camera_properties::camera_plane;


/**********************************
* 控制命令行窗口光标的相关函数
* 支持 Windows 和 Linux 平台
***********************************/
// 设置光标的可视化
inline void CursorVisible(bool visible);
//设置光标的位置，屏幕坐标系
inline void Gotoxy(const short x, const short y);
//清空控制台所有的字符（仅在开头调用一次）
void clearConsole();

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

inline void CursorVisible(bool visible) {
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	cursorInfo.bVisible = visible;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

inline void Gotoxy(const short x, const short y) {
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearConsole()
{
	system("cls");
}
#elif defined(__linux__)
inline void CursorVisible(bool visible) 
{
	if (visible) {
		std::cout << "\033[?25h";
	}
	else {
		std::cout << "\033[?25l";
	}
}

inline void Gotoxy(const short x, const short y) 
{
	std::cout << "\033[" << y << ";" << x << "H";
}

void clearConsole() 
{
	std::cout << "\033[2J\033[H" << std::flush;
}
#endif


void DEMO1()
{
	Camera::init(); //摄像机的初始化
	Cube cube(20); //立方体的初始化，边长设置为20(根据需求改动)
	const double one_angle = 3.14159265 / 180.0; // 1度

	while (true)
	{
		/*关闭光标并且将光标移动到左上角*/
		CursorVisible(false);
		Gotoxy(0, 0);

		/*立方体绕每个轴都旋转1度*/
		cube.rotate_x(-one_angle);
		cube.rotate_y(-one_angle);
		cube.rotate_z(-one_angle);

		Camera::Setcube(cube); //将立方体投影到照相机上
		//Camera::Set_plane_xy();  //绘制x-y坐标系

		Camera::update(); //输出到屏幕
		Camera::clear();  //清空照相机平面内容

		std::this_thread::sleep_for(std::chrono::milliseconds(19));
	}
}

void DEMO2()
{
	Camera::init(80, 80); //摄像机的初始化
	Cube cube(45); //立方体的初始化，边长设置为45

	const std::string surface_pixel = "<.{$~]"; //定义一个表面
	Cube cube1(15, surface_pixel.c_str());

	/* 旋转动画的参数设置 */
	// 定义不同轴的旋转速度（弧度每步）
	const double one_angle = 3.14159265 / 180.0; // 1度
	const double angular_speed_x = one_angle;  // X轴：1度
	const double angular_speed_y = 2 * one_angle;  // Y轴：2度
	const double angular_speed_z = 0.5 * one_angle;  // Z轴：0.5度
	const double time_step = 1.0 / 30.0;  //时间步长控制，基于30 FPS 单位：秒
	double time = 0.0; //用于动画控制的时间

	while (true)
	{
		/*关闭光标并且将光标移动到左上角*/
		CursorVisible(false);
		Gotoxy(0, 0);

		// 计算弧度旋转量，利用不同角速度和时间步长
		double rotation_x = angular_speed_x * sin(time);  // X轴旋转，使用正弦变化产生周期运动
		double rotation_y = angular_speed_y * cos(time);  // Y轴旋转，使用余弦变化产生平滑运动
		double rotation_z = angular_speed_z * sin(time * 0.5);  // Z轴旋转，较慢的正弦变化		
		time += time_step; // 增加时间步长

		// 应用旋转
		cube.rotate_x(rotation_x);
		cube.rotate_y(rotation_y);
		cube.rotate_z(rotation_z);

		cube1.rotate_x(-one_angle * 2);
		cube1.rotate_y(-one_angle * 2);
		cube1.rotate_z(-one_angle * 2);

		Camera::Setcube(cube);
		Camera::Setcube(cube1);
		//Camera::Set_plane_xy();

		Camera::update();
		Camera::clear();

		/*其实我也不知道具体应该设置为多少，但至少知道的是，要确保上述的运算+输出+Sleep(x)的时间之和
		正好是1/30秒=33.33ms*/
		std::this_thread::sleep_for(std::chrono::milliseconds(19));
	}
}

int main()
{
	int choice = 0;
	std::string warn =
		"请将控制台窗口调整至全屏或确保足够大，并使用点阵字体以保持字符长宽比为1。\n";
	std::cout << warn;
	std::cout << "Select a demo:\n";
	std::cout << "1. DEMO1\n";
	std::cout << "2. DEMO2\n";
	std::cout << "Enter your choice: ";
	std::cin >> choice;
	clearConsole();

	switch (choice) 
	{
	case 1:
		DEMO1();
		break;
	case 2:
		DEMO2();
		break;
	default:
		std::cout << "Invalid choice\n";
		break;
	}

	return 0;
}
