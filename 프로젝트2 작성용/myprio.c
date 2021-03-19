#include "sched.h"
static void put_prev_task_myprio(struct rq *rq, struct task_struct *p);
static int select_task_rq_myprio(struct task_struct *p, int cpu, int sd_flag, int flags);
static void set_curr_task_myprio(struct rq *rq);
static void task_tick_myprio(struct rq *rq,struct task_struct *p, int oldprio);
static void switched_to_myprio(struct rq *rq, struct task_struct *p);
void init_myprio_rq(struct myprio_rq *myprio_rq);
static void update_curr_myprio(struct rq *rq);
static void enqueue_task_myprio(struct rq *rq, struct task_struct *p, int flags);
static void dequeue_task_myprio(struct rq *rq, struct task_struct *p, int flags);
static void check_preempt_curr_myprio(struct rq *rq, struct task_struct *p,int flags);
struct task_struct *pick_next_task_myprio(struct rq *rq, struct task_struct *prev);
static void prio_changed_myprio(struct rq *rq, struct task_struct *p, int oldprio);
#define MAX_MYTIME 		4500000011 //about 4.5 sec
#define MAX_MYPRI		10
const struct sched_class myprio_sched_class={
	.next=&fair_sched_class,
	.enqueue_task=&enqueue_task_myprio,
	.dequeue_task=dequeue_task_myprio,
	.check_preempt_curr=check_preempt_curr_myprio,
	.pick_next_task=pick_next_task_myprio,
	.put_prev_task=put_prev_task_myprio,
#ifdef CONFIG_SMP
	.select_task_rq=select_task_rq_myprio,
#endif
	.set_curr_task=set_curr_task_myprio,
	.task_tick=task_tick_myprio,
	.prio_changed=prio_changed_myprio,
	.switched_to=switched_to_myprio,
	.update_curr=update_curr_myprio,
};


void init_myprio_rq (struct myprio_rq *myprio_rq)
{
	unsigned int i;
	myprio_rq->nr_running_total=0;
	for (i = 0; i < MAX_MYPRI; i++) {
		INIT_LIST_HEAD(&myprio_rq->queue[i]);
		myprio_rq->nr_running[i] = 0;
	}
	printk(KERN_INFO "***[MYPRI] Myprio class is online \n");
}

static void update_curr_myprio(struct rq *rq){

	struct task_struct *curr = rq->curr;
	struct sched_myprio_entity *myprio_se = &curr -> myprio;
	unsigned int beforePri;
	unsigned int afterPri;
	unsigned int frontPri;
	unsigned int highest_prio = MAX_MYPRI;
	unsigned int i;
	u64 delta_exec;
	if (curr->sched_class != &myprio_sched_class)
		return;

	for(i=MAX_MYPRI-1 ; i>0 ; i--){
		if(rq->myprio.nr_running[i] >0) {
			highest_prio = i;
		}
	}
	
	//update_rq_clock(rq);
	
	delta_exec = rq_clock_task(rq) - curr->se.exec_start;
	frontPri = myprio_se->myprio-1;
	//printk(KERN_INFO"***[MYPRIO] update_curr_myrr		delta_exec  = %llu\n",delta_exec);
	if(delta_exec > MAX_MYTIME && frontPri == highest_prio)
	{

		beforePri = myprio_se->myprio;
		//delete first
		list_del_init(&curr->myprio.run_list);
		rq->myprio.nr_running[myprio_se->myprio]--;
		myprio_se->myprio = 0;
		curr->mypriority = myprio_se->myprio;
		//enqueue
		afterPri = myprio_se -> myprio;
		list_add_tail(&curr->myprio.run_list, &rq->myprio.queue[myprio_se->myprio]);
		rq->myprio.nr_running[myprio_se->myprio]++;
		//printk(KERN_INFO"***[MYPRIO] update_curr_myrr		pid = %d delta_exec = %llu, beforePRI = %d AfterPRI = %d\n",curr->pid,delta_exec,  beforePri,afterPri);
		resched_curr(rq);
	}


}	


static void enqueue_task_myprio(struct rq *rq, struct task_struct *p, int flags) {
	p->myprio.myprio = p->mypriority;
	list_add_tail(&p->myprio.run_list, &rq->myprio.queue[p->myprio.myprio]);
	rq->myprio.nr_running[p->myprio.myprio]++;
	rq->myprio.nr_running_total++;

	printk(KERN_INFO"***[MYPRI] enqueue: success pri=%u, nr_running=%d, pid=%d\n",p->myprio.myprio, rq->myprio.nr_running_total,p->pid);
}
static void dequeue_task_myprio(struct rq *rq, struct task_struct *p, int flags) 
{
	if(rq->myprio.nr_running_total>0)
	{
		p->mypriority = p->myprio.myprio;
		list_del_init(&p->myprio.run_list);
		rq->myprio.nr_running[p->myprio.myprio]--;
		rq->myprio.nr_running_total--;

		printk(KERN_INFO"\t***[MYPRI] dequeue: success pri=%u, nr_running=%d, pid=%d\n",p->myprio.myprio , rq->myprio.nr_running_total,p->pid);
	}
	else{
	}
	
}
void check_preempt_curr_myprio(struct rq *rq, struct task_struct *p, int flags) {
	printk("***[MYPRI] check_preempt_curr_myprio\n");
}
struct task_struct *pick_next_task_myprio(struct rq *rq, struct task_struct *prev)
{
	int i;
	int highest_prio = MAX_MYPRI;
	struct task_struct* next_p;
	struct sched_myprio_entity* myprio_se = NULL;

	if(rq->myprio.nr_running_total == 0){
		return NULL;
	}
	else {
		 for(i=MAX_MYPRI-1 ; i>=0 ; i--){
			 if(rq->myprio.nr_running[i] >0) {
				highest_prio = i;
			 }
		 }
		
		myprio_se = list_entry(rq->myprio.queue[highest_prio].next, struct sched_myprio_entity, run_list);
		next_p = container_of(myprio_se, struct task_struct, myprio);
		printk(KERN_INFO "\t***[MYPRI] pick_next_task: pri=%u, prev->pid=%d,next_p->pid=%d,nr_running=%d\n",highest_prio , prev->pid, next_p->pid, rq->myprio.nr_running_total);
		return next_p;
	}
		return NULL;	
}
void put_prev_task_myprio(struct rq *rq, struct task_struct *p) {
	printk(KERN_INFO "\t***[MYPRI] put_prev_task: do_nothing, p->pid=%d\n",p->pid);
}
int select_task_rq_myprio(struct task_struct *p, int cpu, int sd_flag, int flags){return task_cpu(p);}
void set_curr_task_myprio(struct rq *rq){
	printk(KERN_INFO"***[MYPRI] set_curr_task_myprio\n");
}
void task_tick_myprio(struct rq *rq, struct task_struct *p, int queued) {
	update_curr_myprio(rq);
}
void prio_changed_myprio(struct rq *rq, struct task_struct *p, int oldprio) { }
/*This routine is called when a task migrates between classes*/
void switched_to_myprio(struct rq *rq, struct task_struct *p)
{
	resched_curr(rq);
}

