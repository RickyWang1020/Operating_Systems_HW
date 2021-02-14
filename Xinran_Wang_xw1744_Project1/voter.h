#ifndef VOTER_H
#define VOTER_H

#ifndef STRUCT_VOTER
#define STRUCT_VOTER
typedef struct Voter{
	char first[256];
	char last[256];
	int RIN;
	int zip_code;
    int voted;
    struct Voter *next;
} Voter;
#endif

#ifndef STRUCT_VOTER_HOLDER
#define STRUCT_VOTER_HOLDER
typedef struct Voter_holder{
	struct Voter *voter;
    struct Voter_holder *next;
} Voter_holder;
#endif

#ifndef STRUCT_POSTCODE
#define STRUCT_POSTCODE
typedef struct Postcode{
	int code;
	int num_of_voter;
	struct Voter_holder *voter_holder;
    struct Postcode *next;
} Postcode;
#endif

#ifndef STRUCT_HASH_AND_LL
#define STRUCT_HASH_AND_LL
typedef struct Hash_and_ll{
	struct Voter *voter;
    struct Postcode *postcode;
} Hash_and_ll;
#endif

extern int people_in_hash;
extern int people_voted;
extern int table_size;

struct Hash_and_ll regist(Voter *table, Postcode *post_ll, int rin);
Postcode *insert_voted_to_ll(Postcode *post_ll, Voter *vote_people);
struct Hash_and_ll delete(Voter *table, Postcode *post_ll, int rin);
Postcode *delete_voted_from_ll(Postcode *post_ll, Voter *vote_people);
void print_by_postcode(Postcode *post_ll, int zip);
void print_ll(Postcode *post_ll);
Postcode *sort_ll(Postcode *post_ll);
Postcode *free_ll_element(Postcode *post_ll);

#endif