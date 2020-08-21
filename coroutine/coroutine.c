#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine.h"

//创建协程调度器
schedule_t *schedule_create() {
	schedule_t *s = (schedule_t*)malloc(sizeof(schedule_t));
	//初始化
	if ( s != NULL ) {
		s->coroutines = (coroutine_t**)malloc(sizeof(coroutine_t*)*CORSZ);
		memset(s->coroutines, 0x00, sizeof(coroutine_t*)*CORSZ);

		s->current_id = -1;
		s->max_id = 0;
	}	

	return s;
}

// 协程执行函数
 static void *main_fun(schedule_t *s)
 {
	int id = s->current_id;
	//如果有协程正在运行
	if ( id != -1 ) {
		coroutine_t *c = s->coroutines[s->current_id];
		c->call_back(s, c->args);
		c->state = DEAD; // 目标函数执行完毕，就进入DEAD状态
		s->current_id = -1; //运行完置为-1
	}
}

// 创建协程，返回当前协程在调度器的下标
int coroutine_create(schedule_t *s, void *(*call_back)(schedule_t *, void *args), void *args) {
	int i;
	//定义一个协程结构体
	coroutine_t *c = NULL;
	//将协程放入调度器
	for (i=0; i<s->max_id; i++) {
		c = s->coroutines[i];
		//找到DEAD状态的位置放入
		if ( c->state == DEAD )
			break;
	}	
	//扩容
	if ( i==s->max_id || c == NULL ) {
		s->coroutines[i] = (coroutine_t*)malloc(sizeof(coroutine_t));
		s->max_id ++;
	}
	//初始化
	c = s->coroutines[i];
	c->call_back = call_back;
	c->args = args;
	c->state = READY;
	//保存当前状态
	getcontext(&c->ctx);
	//重定义栈
	c->ctx.uc_stack.ss_sp = c->stack;
	c->ctx.uc_stack.ss_size = STACKSZ;
	c->ctx.uc_stack.ss_flags = 0;
	c->ctx.uc_link = &s->ctx_main;
	//执行新的回调函数
	makecontext(&c->ctx, (void (*)())&main_fun, 1, s);

	return i;
}

//获取协程状态
static enum State get_states(schedule_t *s,int id){
	coroutine_t *c = s->coroutines[id];
	if ( c == NULL ) 
		return DEAD;
	return c->state;
}

//启动协程
void coroutine_running(schedule_t *s, int id) {
	int st = get_states(s, id);
	if ( st == DEAD)
		return;

	coroutine_t *c = s->coroutines[id];
	//更改协程状态为运行态
	c->state = RUNNING;
	s->current_id = id;
	//恢复cpu运行当前协程，保存主流程
	swapcontext(&s->ctx_main, &c->ctx);
	}

//让出CPU（get）
void coroutine_yield(schedule_t *s) {
	if ( s->current_id != -1 ) {
		coroutine_t *c = s->coroutines[s->current_id];
		c->state = SUSPEND;
		s->current_id = -1;
		//保存当前c状态，运行主流程
		swapcontext(&c->ctx, &s->ctx_main);
	}
}

//恢复CPU（set）
void coroutine_resume(schedule_t *s, int id) {
	coroutine_t *c = s->coroutines[id];
	if ( c!=NULL && c->state == SUSPEND ) {
		c->state = RUNNING;
		s->current_id = id;
		//保存主流程，运行c
		swapcontext(&s->ctx_main, &c->ctx);
	}	
}

//删除协程
static void delete_coroutine(schedule_t *s, int id) {
	coroutine_t *c = s->coroutines[id];
	if ( c != NULL ) {
		free(c);
		s->coroutines[id] = NULL;
	}
}

//释放调度器
void schedule_destroy(schedule_t *s) {
	int i;
	for (i=0; i<s->max_id; i++) {
		delete_coroutine(s, i);
	}

	free(s->coroutines);
	free(s);
}

// 判断是否所有协程都运行完了
int schedule_finished(schedule_t *s) {
	if ( s->current_id != -1 )
		return 0;
	int i;
	for (i=0; i<s->max_id; i++) {
		coroutine_t *c = s->coroutines[i];
		if ( c->state != DEAD )
			return 0;
	}	

	return 1;
}
