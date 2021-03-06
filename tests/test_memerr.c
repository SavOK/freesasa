#include <stdlib.h>
#include <check.h>
int n_fails = 0;
int fail_freq = 1;

// use these to test what happens when malloc and realloc fail,
// the global variables malloc_fail_freq and realloc_fail_freq
// can be set to let 
void*
broken_malloc(size_t s)
{
    ++n_fails;
    if (n_fails % fail_freq == 0) return NULL;
    return malloc(s);
}

void*
broken_realloc(void * ptr, size_t s)
{
    ++n_fails;
    if (n_fails % fail_freq == 0) return NULL;
    return realloc(ptr,s);
}

void*
broken_strdup(const char *s)
{
    ++n_fails;
    if (n_fails % fail_freq == 0) return NULL;
    return strdup(s);
}

void
set_fail_freq(int freq) {
    if (freq < 1) freq = 1;
    fail_freq = freq;
    n_fails = 0;
}

#define malloc(m) broken_malloc(m)
#define realloc(m,n) broken_realloc(m,n)
#define strdup(s) broken_strdup(s)


#define FREESASA_NB_CHUNK 1 // to force som reallocs to take place
#include "whole_lib_one_file.c"

int int_array[6] = {0,1,2,3,4,5};
char str_array[][2] = {"A","B","C","D","E","F"};
double v[18] = {0,0,0, 1,1,1, -1,1,-1, 2,0,-2, 2,2,0, -5,5,5};
const double r[6]  = {4,2,2,2,2,2};
double dummy[20];
struct coord_t coord = {.xyz = v, .n = 6, .is_linked = 0};
struct atom *a;
struct freesasa_structure structure = {
    .a = &a,
    .xyz = &coord,
    .number_atoms = 6,
    .number_residues = 1,
    .number_chains = 1, 
    .model = 1,
    .chains = "A",
    .res_first_atom = int_array,
    .res_desc = (char**)str_array
};
struct file_range range = {.begin = 0, .end = 1};
struct cell a_cell = {.nb = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                             NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
                      .atom = int_array, .n_nb=0, .n_atoms = 0};
struct cell_list a_cell_list = {.cell = &a_cell, .n = 1, .nx = 1, .ny =1, .nz = 1,
                                .d = 20, .x_min = 0, .x_max = 1, 
                                .y_min = 0, .y_max = 1,
                                .z_min = 0, .z_max = 1};

START_TEST (test_coord) 
{
    set_fail_freq(1000);
    coord_t *coord_dyn = freesasa_coord_new();
    set_fail_freq(1);
    freesasa_set_verbosity(FREESASA_V_SILENT);
    ck_assert_ptr_eq(freesasa_coord_new(),NULL);
    ck_assert_ptr_eq(freesasa_coord_copy(&coord),NULL);
    ck_assert_ptr_eq(freesasa_coord_new_linked(v,1),NULL);
    ck_assert_int_eq(freesasa_coord_append(coord_dyn,v,1),FREESASA_FAIL);
    ck_assert_int_eq(freesasa_coord_append_xyz(coord_dyn,v,v+1,v+2,1),FREESASA_FAIL);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
    freesasa_coord_free(coord_dyn);
}
END_TEST

START_TEST (test_structure)
{
    FILE *file = fopen(DATADIR "1ubq.pdb","r");
    int n;
    freesasa_set_verbosity(FREESASA_V_SILENT);
    ck_assert_ptr_ne(file, NULL);
    ck_assert_ptr_eq(freesasa_structure_new(), NULL);
    ck_assert_ptr_eq(from_pdb_impl(file,range, NULL, 0), NULL);
    for (int i = 1; i < 100; ++i) {
        set_fail_freq(i);
        rewind(file);
        ck_assert_ptr_eq(freesasa_structure_from_pdb(file, NULL, 0), NULL);
    }
    fclose(file);
    
    file = fopen(DATADIR "2jo4.pdb", "r");
    ck_assert_ptr_ne(file,NULL);
    for (int i = 1; i < 50; ++i) {
        set_fail_freq(i);
        rewind(file);
        ck_assert_ptr_eq(freesasa_structure_array(file, &n, NULL, FREESASA_SEPARATE_MODELS),NULL);
        rewind(file);
        ck_assert_ptr_eq(freesasa_structure_array(file, &n, NULL, FREESASA_SEPARATE_MODELS | FREESASA_SEPARATE_CHAINS),NULL);
    }
    set_fail_freq(1);
    fclose(file);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
}
END_TEST

START_TEST (test_nb) 
{
    freesasa_set_verbosity(FREESASA_V_SILENT);
    ck_assert_ptr_eq(cell_list_new(1,&coord),NULL);
    ck_assert_int_eq(fill_cells(&a_cell_list,&coord),FREESASA_FAIL);
    ck_assert_ptr_eq(freesasa_nb_alloc(10),NULL);
    for (int i = 1; i < 50; ++i) {
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_nb_alloc(2*i),NULL);
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_nb_new(&coord,r),NULL);
    }
    set_fail_freq(1);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
}
END_TEST

START_TEST (test_alg)
{
    // First check that the input actually gives a valid calculation
    set_fail_freq(10000);
    ck_assert_int_eq(freesasa_lee_richards(dummy, &coord, r, NULL), FREESASA_SUCCESS);
    ck_assert_int_eq(freesasa_shrake_rupley(dummy, &coord, r, NULL), FREESASA_SUCCESS);
    
    freesasa_set_verbosity(FREESASA_V_SILENT);
    for (int i = 1; i < 50; ++i) {
        set_fail_freq(i);
        ck_assert_int_eq(freesasa_lee_richards(dummy,&coord,r,NULL),FREESASA_FAIL);
        set_fail_freq(i);
        ck_assert_int_eq(freesasa_shrake_rupley(dummy,&coord,r,NULL),FREESASA_FAIL);
    }
    set_fail_freq(1);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
}
END_TEST

START_TEST (test_classifier) 
{
    freesasa_set_verbosity(FREESASA_V_SILENT);

    set_fail_freq(1);
    ck_assert_ptr_eq(classifier_types_new(),NULL);
    ck_assert_ptr_eq(classifier_residue_new("A"),NULL);
    ck_assert_ptr_eq(classifier_config_new(),NULL);

    for (int i = 1; i < 5; ++i) {
        set_fail_freq(10000);
        struct classifier_types *types = classifier_types_new();
        struct classifier_residue *res = classifier_residue_new("ALA");
        struct classifier_config *cfg = classifier_config_new();

        if (i < 3) {
            set_fail_freq(i);
            ck_assert_int_eq(add_class(types,"A"),FREESASA_FAIL);        
            set_fail_freq(i);
            ck_assert_int_eq(add_type(types,"a","A",1.0),FREESASA_FAIL);
        } 
        set_fail_freq(i);
        ck_assert_int_eq(add_atom(res,"A",1.0,0),FREESASA_FAIL);
        set_fail_freq(i);
        ck_assert_int_eq(add_residue(cfg,"A"),FREESASA_FAIL);
        classifier_types_free(types);
        classifier_residue_free(res);
        classifier_config_free(cfg);
    }
    // don't test all levels, but make sure errors in low level
    // allocation propagates to the interface
    FILE *config = fopen(SHAREDIR "naccess.config","r");
    ck_assert_ptr_ne(config, NULL);
    for (int i = 1; i < 50; ++i) {
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_classifier_from_file(config),NULL);
        rewind(config);
    }
    fclose(config);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
}
END_TEST

START_TEST (test_selector) 
{
    set_fail_freq(1000000);
    freesasa_parameters p = freesasa_default_parameters;
    p.shrake_rupley_n_points = 10;
    FILE *file = fopen(DATADIR "1ubq.pdb", "r");
    freesasa_structure *s = freesasa_structure_from_pdb(file, NULL, 0);
    freesasa_result *result = freesasa_calc_structure(s, &p);
    double area;
    char name[FREESASA_MAX_SELECTION_NAME];

    freesasa_set_verbosity(FREESASA_V_SILENT);
    for (int i = 1; i < 17; ++i) { 
        /* This is a pretty short expression. Have not verified that
           it's actually exactly 17 memory allocations, but we have
           success with a frequencey of 18, and 17 seems about right. */
        set_fail_freq(i);
        ck_assert_int_eq(freesasa_select_area("s, resn ALA and not resi 1-20", name, &area, s, result),
                         FREESASA_FAIL); // this expression should come across most allocations
    }
    freesasa_set_verbosity(FREESASA_V_NORMAL);
    fclose(file);
    freesasa_result_free(result);
    freesasa_structure_free(s);
}
END_TEST

START_TEST (test_api) 
{
    freesasa_parameters p = freesasa_default_parameters;
    p.shrake_rupley_n_points = 20; // so the loop below will be fast

    freesasa_set_verbosity(FREESASA_V_SILENT);
    for (int i = 1; i < 50; ++i) {
        p.alg = FREESASA_SHRAKE_RUPLEY;
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_calc(&coord, r, &p), NULL);
        p.alg = FREESASA_LEE_RICHARDS; 
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_calc(&coord, r, &p), NULL);
    }

    FILE *file = fopen(DATADIR "1ubq.pdb","r");
    set_fail_freq(10000);
    freesasa_structure *s=freesasa_structure_from_pdb(file, NULL, 0);
    ck_assert_ptr_ne(s,NULL);
    for (int i = 1; i < 256; i *= 2) { //try to spread it out without doing too many calculations
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_calc_structure(s, NULL), NULL);
        set_fail_freq(i);
        ck_assert_ptr_eq(freesasa_structure_get_chains(s, "A"), NULL);
    }
    set_fail_freq(1);
    freesasa_structure_free(s);
    fclose(file);
    freesasa_set_verbosity(FREESASA_V_NORMAL);
}
END_TEST

int main(int argc, char **argv) {
    Suite *s = suite_create("Test that null-returning malloc breaks program gracefully.");
    
    TCase *tc = tcase_create("Basic");
    tcase_add_test(tc,test_coord);
    tcase_add_test(tc,test_structure);
    tcase_add_test(tc,test_nb);
    tcase_add_test(tc,test_alg);
    tcase_add_test(tc,test_classifier);
    tcase_add_test(tc,test_selector);
    tcase_add_test(tc,test_api);
    
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr,CK_VERBOSE);
    
    return (srunner_ntests_failed(sr) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
