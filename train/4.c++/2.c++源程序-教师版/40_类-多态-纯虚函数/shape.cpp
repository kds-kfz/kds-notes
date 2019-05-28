
写一个基类Shape(形状) ，是一个抽象类
class Shape {
	double area;	//表面积
	double volume;	//体积
	virtual double getArea() = 0;
				   getVolume() = 0;
};
正方体
class Cube : public Shape
{
};
球体
class Ball : public Shape
{
};
圆柱体
class Cylinder : public Shape
{
};
做一个选项，选择想要作出的图形
1, 正方体
2, 球体
3, 圆柱体

