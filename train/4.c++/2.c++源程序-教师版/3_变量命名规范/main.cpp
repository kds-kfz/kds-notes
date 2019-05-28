命名规范：数字，字母，下划线，首字母不能是数字
标准库内一些名称是以下划线开头 _exit __FILE__
自定义名字的时候，尽量不要在开头加下划线

一些通用的命名规范：

大驼峰命名法
int StudentCount;	结构体名字

小驼峰命名法
int studentCount;	一般是变量或函数的名字

下划线命名法
int student_count;	变量或函数名字	linux C里

匈牙利命名法	把变量的属性类型写在名字里

	属性
	g_	全局变量
	c_	const
	s_	static
	m_	成员变量

	类型
	int		i
	float	f
	char	c
	指针	p
	函数	fn
	句柄	h
	...

