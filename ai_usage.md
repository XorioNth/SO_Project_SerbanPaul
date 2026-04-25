AI used: Claude Sonnet 4.6 Adaptive

Prompt: write a function in c with the following header int parse_condition(const char *input, char *field, char *op, char *value); which splits a filed:operator:value string into its three parts
example severity:>=:2 category:==:road
```
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (!input || !field || !op || !value)
        return -1;

    const char *first_colon = strchr(input, ':');
    if (!first_colon)
        return -1;

    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon)
        return -1;

    /* Reject a trailing third colon in the value portion */
    if (strchr(second_colon + 1, ':'))
        return -1;

    /* field: from start up to the first colon */
    size_t field_len = first_colon - input;
    if (field_len == 0) return -1;
    memcpy(field, input, field_len);
    field[field_len] = '\0';

    /* op: between the two colons */
    size_t op_len = second_colon - (first_colon + 1);
    if (op_len == 0) return -1;
    memcpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    /* value: everything after the second colon */
    size_t value_len = strlen(second_colon + 1);
    if (value_len == 0) return -1;
    memcpy(value, second_colon + 1, value_len);
    value[value_len] = '\0';

    return 0;
}
```
Follow up question: What about strtok?

Answer: strtok modifies the input. It writes \0 over the delimiters in-place, so you'd need to strdup the input first just to preserve it. The pointer-arithmetic approach reads input without touching it.
strtok silently skips consecutive delimiters. strtok("field::value", ":") skips the empty token and returns "field", then "value" — it never tells you the operator was missing. Your validation would have to do extra work to catch that. strchr makes the two-colon structure explicit.
strtok is not reentrant. It stores state in a hidden static variable, so two interleaved calls (e.g. nested loops, threads) corrupt each other. You'd need the POSIX-only strtok_r, which is less portable.

What I learned: the method above is safer than strtok. I have learned new stuff about the fucntion strtok, some unsafe things that happne in the background that I wasn't aware of.

Prompt:Write a function in C with the following header:
int match_condition(Report *r, const char *field, const char *op, const char *value);
the Report refers to this:
```
typedef struct report {
    int report_id;
char inspector_name[50];
double X, Y;
char category[50];
int severity;
time_t timestamp;
char description[256];
}report;
```
For field, the supported values are: severity, category, inspector, timestamp
For the operation field, the supported values are:==, !=, <, <=, >, >=
verify with the corresponding field, if the first field is severity, check that the severity of the report is equal to the given value (if the operator is ==, in this case)
this function returns 1 if the record satisfies, 0 if not.
```
int match_condition(report *r, const char *field, const char *op, const char *value) {
    if (!r || !field || !op || !value) return 0;

    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<")  == 0) return r->severity <  val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">")  == 0) return r->severity >  val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    }
    else if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        return 0; // <, <=, >, >= nu au sens pentru string
    }
    else if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector_name, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        return 0;
    }
    else if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, "<")  == 0) return r->timestamp <  val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">")  == 0) return r->timestamp >  val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
    }

    return 0; 
}
```
What I changed: implemented >, < comparisons for strings too, such that we can compare lexicographically
```
else if (strcmp(field, "category") == 0) {
    int cmp = strcmp(r->category, value);
    if (strcmp(op, "==") == 0) return cmp == 0;
    if (strcmp(op, "!=") == 0) return cmp != 0;
    if (strcmp(op, "<")  == 0) return cmp <  0;
    if (strcmp(op, "<=") == 0) return cmp <= 0;
    if (strcmp(op, ">")  == 0) return cmp >  0;
    if (strcmp(op, ">=") == 0) return cmp >= 0;
}
else if (strcmp(field, "inspector") == 0) {
    int cmp = strcmp(r->inspector_name, value);
    if (strcmp(op, "==") == 0) return cmp == 0;
    if (strcmp(op, "!=") == 0) return cmp != 0;
    if (strcmp(op, "<")  == 0) return cmp <  0;
    if (strcmp(op, "<=") == 0) return cmp <= 0;
    if (strcmp(op, ">")  == 0) return cmp >  0;
    if (strcmp(op, ">=") == 0) return cmp >= 0;
}
```

