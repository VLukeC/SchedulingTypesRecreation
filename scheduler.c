#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// total jobs
int numofjobs = 0;

struct job
{
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    int checked;
    int startTime;
    int completion;
    struct job *next;
};

// the workload list
struct job *head = NULL;

void append_to(struct job **head_pointer, int arrival, int length, int tickets)
{
    struct job *newJob = malloc(sizeof(struct job));
    newJob->arrival = arrival;
    newJob->length = length;
    newJob->tickets = tickets;
    newJob->checked = 0;
    newJob->next = NULL;
    newJob->id = numofjobs;
    numofjobs++;
    if (*head_pointer == NULL)
    {
        *head_pointer = newJob;
    }
    else
    {
        struct job *temp = *head_pointer;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newJob;
    }
    return;
};

void read_job_config(const char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets = 0;

    char *delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    // TODO: if the file is empty, we should just exit with error
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = 0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }
    fclose(fp);
    if (line)
        free(line);
    if (numofjobs == 0)
    {
        printf("\nFile Empty");
        exit(1);
    }
}

void policy_SJF()
{
    printf("Execution trace with SJF:\n");
    int time = 0;
    int finished = 0;

    if (head && head->arrival > 0)
        time = head->arrival;

    for (struct job *p = head; p; p = p->next)
        p->checked = 0;

    while (finished < numofjobs)
    {
        struct job *best = NULL;
        for (struct job *p = head; p; p = p->next)
        {
            if (!p->checked && p->arrival <= time)
            {
                if (!best || p->length < best->length)
                {
                    best = p;
                }
            }
        }
        // if not arrived job is free move to next arrival
        if (!best)
        {
            int next_arrival = INT_MAX;
            for (struct job *p = head; p; p = p->next)
            {
                if (!p->checked && p->arrival < next_arrival)
                {
                    next_arrival = p->arrival;
                }
            }
            if (next_arrival == INT_MAX)
                break;
            time = next_arrival;
            continue;
        }
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, best->id, best->arrival, best->length);
        best->startTime = time;
        time += best->length;
        best->completion = time;
        best->checked = 1;
        finished++;
    }
    printf("End of execution with SJF.\n");
}

void analysis_SJF()
{
    printf("Begin analyzing SJF:\n");
    double avgRT = 0;
    double avgTA = 0;
    for (struct job *p = head; p; p = p->next)
    {
        int response_time = p->startTime - p->arrival;
        int turnaround = p->completion - p->arrival;
        int wait = response_time; // same for non-pre-emptive
        avgRT += response_time;
        avgTA += turnaround;
        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", p->id, response_time, turnaround, wait);
    }
    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avgRT / numofjobs, avgTA / numofjobs, avgRT / numofjobs);
    printf("End analyzing SJF.\n");
}

void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy

    printf("End of execution with STCF.\n");
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    int time = 0;
    int finished = 0;
    int remaining[numofjobs];
    int started[numofjobs];
    struct job *jobs[numofjobs];

    int idx = 0;
    for (struct job *p = head; p; p = p->next)
    {
        jobs[idx] = p;
        remaining[idx] = p->length;
        started[idx] = 0;
        idx++;
    }

    int queue[numofjobs];
    int front = 0, rear = 0;
    int next_job_idx = 0;
    if (head && head->arrival > 0)
        time = head->arrival;

    while (finished < numofjobs)
    {
        // enqueue all jobs that have arrived by current time
        for (; next_job_idx < numofjobs && jobs[next_job_idx]->arrival <= time; next_job_idx++)
        {
            queue[rear++] = next_job_idx;
        }

        // if no job is ready, jump to next arrival
        if (front == rear)
        {
            if (next_job_idx < numofjobs)
            {
                time = jobs[next_job_idx]->arrival;
                continue;
            }
            else
                break;
        }

        // dequeue next job
        int jid = queue[front++];
        struct job *curr = jobs[jid];
        int run_time = min(slice, remaining[jid]);

        if (!started[jid])
        {
            curr->startTime = time;
            started[jid] = 1;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               time, curr->id, curr->arrival, run_time);

        // advance simulation
        time += run_time;
        remaining[jid] -= run_time;

        // enqueue any jobs that arrived during this time
        for (; next_job_idx < numofjobs && jobs[next_job_idx]->arrival <= time; next_job_idx++)
        {
            queue[rear++] = next_job_idx;
        }

        // if job still not finished, put it at end of queue
        if (remaining[jid] > 0)
        {
            queue[rear++] = jid;
        }
        else
        {
            curr->completion = time;
            finished++;
        }
    }

    printf("End of execution with RR.\n");
}

void analysis_RR()
{
    printf("Begin analyzing RR:\n");

    double total_response = 0;
    double total_turnaround = 0;
    double total_wait = 0;

    for (struct job *p = head; p; p = p->next)
    {
        int response = p->startTime - p->arrival;
        int turnaround = p->completion - p->arrival;
        int wait = turnaround - p->length;

        total_response += response;
        total_turnaround += turnaround;
        total_wait += wait;

        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
               p->id, response, turnaround, wait);
    }

    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
           total_response / numofjobs,
           total_turnaround / numofjobs,
           total_wait / numofjobs);

    printf("End analyzing RR.\n");
}

void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");
}

/* =========================
   Implemented: FIFO policy
   ========================= */
void policy_FIFO()
{
    printf("Execution trace with FIFO:\n");

    int time = 0;
    int finished = 0;

    if (head && head->arrival > 0)
        time = head->arrival;

    for (struct job *p = head; p; p = p->next)
        p->checked = 0;

    while (finished < numofjobs)
    {
        // pick the earliest arrived job among those not yet run
        struct job *best = NULL;
        for (struct job *p = head; p; p = p->next)
        {
            if (!p->checked && p->arrival <= time)
            {
                if (!best || p->arrival < best->arrival ||
                    (p->arrival == best->arrival && p->id < best->id))
                {
                    best = p;
                }
            }
        }

        // if nothing has arrived yet, jump to the next arrival
        if (!best)
        {
            int next_arrival = INT_MAX;
            for (struct job *p = head; p; p = p->next)
            {
                if (!p->checked && p->arrival < next_arrival)
                {
                    next_arrival = p->arrival;
                }
            }
            if (next_arrival == INT_MAX)
                break;
            time = next_arrival;
            continue;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               time, best->id, best->arrival, best->length);

        best->startTime = time;
        time += best->length;
        best->completion = time;
        best->checked = 1;
        finished++;
    }

    printf("End of execution with FIFO.\n");
}

int main(int argc, char **argv)
{

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;

    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0)
    {
        policy_FIFO();
        if (analysis == 1)
        {
            // analysis_SJF computes generic non preemptive metrics already stored by the policy
            analysis_SJF();
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if (analysis == 1)
        {
            analysis_SJF();
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);
        if (analysis == 1)
        {
            analysis_RR();
        }
    }

    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

    exit(0);
}
