#ifndef NONCPYABLE_H
#define NONCPYABLE_H

class noncpyable {
    public:
	noncpyable() = default;//要求系统默认合成
	noncpyable(const noncpyable& n) = delete;//禁止合成拷贝构造
	noncpyable& operator=(const noncpyable& ) = delete;//禁止赋值
};

#endif
