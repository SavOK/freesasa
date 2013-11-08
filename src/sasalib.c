/*
  Copyright Simon Mitternacht 2013.

  This file is part of Sasalib.
  
  Sasalib is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Sasalib is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Sasalib.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sasa.h"
#include "pdbutil.h"
#include "sasalib.h"
#include "srp.h"
#include "classify.h"

#define NBUF 100
typedef struct {
    int *class;
    int *residue;
} class_t;

typedef struct {
    double *sasa;
    double total;
    double *class;
    double *residue;
} result_t;

struct sasalib_ {
    sasalib_algorithm alg;
    double *r;
    coord_t *coord;
    class_t *class;
    int n_atoms;
    int n_sr;
    double d_lr;
    int n_threads;
    double elapsed_time;
    int owns_r;
    int calculated;
    result_t *result;
    char proteinname[SASALIB_NAME_LIMIT];
}; 

const sasalib_t sasalib_def_param = {
    .alg = SASALIB_SHRAKE_RUPLEY,
    .r = NULL,
    .coord = NULL,
    .class = NULL,
    .result = NULL,
    .n_atoms = 0,
    .n_sr = SASALIB_DEF_SR_N,
    .d_lr = SASALIB_DEF_LR_D,
    .n_threads = 1,
    .elapsed_time = 0,
    .owns_r = 0,
    .calculated = 0,
    .proteinname = "undef"
};

const char *sasalib_alg_names[] = {"Lee & Richards", "Shrake & Rupley"};

static class_t* sasalib_classify_structure(const structure_t *p)
{
    assert(p != NULL);
    assert(structure_n(p) >= 0 && "Trying to analyze illegal structure.");
    
    size_t n = structure_n(p);
    
    class_t* c = (class_t*)malloc(sizeof(class_t));
    c->class = (int*)malloc(sizeof(int)*n);
    c->residue = (int*)malloc(sizeof(int)*n);

    for (int i = 0; i < n; ++i) {
	const char *res_name = structure_atom_res_name(p,i);
	const char *atom_name = structure_atom_name(p,i);
	c->class[i] = classify_class(res_name,atom_name);
	c->residue[i] = classify_residue(res_name);
    }

    return c;
}

static void sasalib_class_free(class_t *c) 
{
    if (! c) return;
    free(c->class);
    free(c->residue);
    free(c);
}

static result_t* sasalib_result_new(size_t n_atoms)
{
    result_t *r = (result_t*)malloc(sizeof(result_t));

    int nc = classify_nclasses();
    int nr = classify_nresiduetypes();
    
    r->sasa = (double*)malloc(sizeof(double)*n_atoms);
    r->class = (double*)malloc(sizeof(double)*nc);
    r->residue = (double*)malloc(sizeof(double)*nr);

    for (int i = 0; i < nc; ++i) r->class[i] = 0;
    for (int i = 0; i < nr; ++i) r->residue[i] = 0;
    
    return r;
}

static void sasalib_result_free(result_t *r)
{
    if (! r) return;
    free(r->sasa);
    free(r->class);
    free(r->residue);
    free(r);
}

static void sasalib_get_class_result(sasalib_t *s, structure_t *p)
{
    if (s->class) sasalib_class_free(s->class);
    s->class = sasalib_classify_structure(p);
    
    for (size_t i = 0; i < structure_n(p); ++i) {
        s->result->class[s->class->class[i]] += s->result->sasa[i];
	s->result->residue[s->class->residue[i]] += s->result->sasa[i];
    }
}

int sasalib_calc(sasalib_t *s, const coord_t *c, const double *r)
{
    struct timeval t1, t2;
    int res = 0;

    gettimeofday(&t1,NULL);

    if (s->result) sasalib_result_free(s->result);
    result_t *result;
    s->result = result = sasalib_result_new(s->n_atoms);

    switch(s->alg) {
    case SASALIB_SHRAKE_RUPLEY:
        res = sasa_shrake_rupley(result->sasa, c, r, 
				 s->n_sr, s->n_threads);
        break;
    case SASALIB_LEE_RICHARDS:
        res = sasa_lee_richards(result->sasa, c, r, 
				s->d_lr, s->n_threads);
	break;
    default:
        fprintf(stderr,"Error: no SASA algorithm specified.\n");
        return 1;
    }
    result->total = 0;
    for (int i = 0; i < coord_n(c); ++i) {
	result->total += result->sasa[i];
    }

    gettimeofday(&t2,NULL);
    
    s->elapsed_time = (t2.tv_sec-t1.tv_sec); 
    s->elapsed_time += (t2.tv_usec-t1.tv_usec) / 1e6; // s
    
    s->calculated = 1;

    return res;
}

sasalib_t* sasalib_init()
{
    sasalib_t *s = (sasalib_t*) malloc(sizeof(sasalib_t));
    *s = sasalib_def_param;
    return s;
}

void sasalib_copy_param(sasalib_t *target, const sasalib_t *source)
{
    target->alg = source->alg;
    target->n_sr = source->n_sr;
    target->d_lr = source->d_lr;
    target->n_threads = source->n_threads;
}

void sasalib_free(sasalib_t *s)
{
    if (! s) return;
    if (s->owns_r) {
        free(s->r);
    }
    if (s->class) sasalib_class_free(s->class);
    if (s->result) sasalib_result_free(s->result);
    if (s->coord) coord_free(s->coord);
    free(s);
}

int sasalib_calc_coord(sasalib_t *s, const double *coord, 
                       const double *r, size_t n)
{
    s->n_atoms = n;
    coord_t *c = coord_new_linked(coord,n);
    int res = sasalib_calc(s,c,r);
    coord_free(c);
    return res;
}

int sasalib_calc_pdb(sasalib_t *s, FILE *pdb_file)
{
    structure_t *p = structure_init_from_pdb(pdb_file);
    s->n_atoms = structure_n(p);

    //calc OONS radii
    if (s->r) free(s->r);
    s->r = (double*) malloc(sizeof(double)*structure_n(p));
    s->owns_r = 1;
    structure_r_def(s->r,p);

    int res = sasalib_calc(s,structure_xyz(p),s->r);
    if (!res) sasalib_get_class_result(s,p);
    structure_free(p);
    return res;
}

int sasalib_link_coord(sasalib_t *s, const double *coord,
                       double *r, size_t n) 
{
    s->coord = coord_new_linked(coord,n);

    if (s->r) free(s->r);
    s->r = r;
    s->owns_r = 0;

    return 0;
}

int refresh(sasalib_t *s)
{
    if (! s->coord || ! s->r ) {
        fprintf(stderr,"Error: trying to refresh unitialized sasalib_t-object.\n");
        return 1;
    }
    sasalib_calc(s,s->coord,s->r);
    return 0;
}

int sasalib_set_algorithm(sasalib_t *s, sasalib_algorithm alg)
{
    if (alg == SASALIB_SHRAKE_RUPLEY || alg == SASALIB_LEE_RICHARDS) { 
        s->alg = alg;
        return 0;
    }
    return 1;
}

sasalib_algorithm sasalib_get_algorithm(const sasalib_t *s)
{
    return s->alg;
}

const char* sasalib_algorithm_name(const sasalib_t *s)
{
    assert(s->alg == SASALIB_SHRAKE_RUPLEY || s->alg == SASALIB_LEE_RICHARDS);
    return sasalib_alg_names[s->alg];
}

int sasalib_set_sr_points(sasalib_t *s, int n) {
    if (srp_n_is_valid(n)) { 
        s->n_sr = n;
        return 0;
    }
    fprintf(stderr,"Error: Number of test-points has to be"
	    " one of the following values:\n  ");
    srp_print_n_opt(stderr);
    fprintf(stderr,"       Proceeding with default value (%d)\n",
            SASALIB_DEF_SR_N);
    s->n_sr = SASALIB_DEF_SR_N;
    return 1;
}

int sasalib_get_sr_points(const sasalib_t* s)
{
    if (s->alg == SASALIB_SHRAKE_RUPLEY) return s->n_sr;
    return -1;
}

int sasalib_set_lr_delta(sasalib_t *s, double d)
{
    if (d > 0 && d < 5.01) {
        s->d_lr = d;
        return 0;
    }
    fprintf(stderr,"Error: slice width has to lie between 0 and 5 Å\n");
    fprintf(stderr,"       Proceeding with default value (%f)\n",
            SASALIB_DEF_LR_D);
    s->d_lr = SASALIB_DEF_LR_D;
    return 1;
}

int sasalib_get_lr_delta(const sasalib_t *s)
{
    if (s->alg == SASALIB_LEE_RICHARDS) return s->d_lr;
    return -1.0;
}

#ifdef PTHREADS
int sasalib_set_nthreads(sasalib_t *s,int n)
{
    if ( n <= 0) {
        fprintf(stderr,"Error: Number of threads has to be positive.\n");
        return 1;
    }
    s->n_threads = n;
    return 0;
}

int sasalib_get_nthreads(const sasalib_t *s)
{
    return s->n_threads;
}
#endif

size_t sasalib_n_atoms(const sasalib_t *s)
{
    return s->n_atoms;
}

double sasalib_area_total(const sasalib_t *s)
{
    if (! s->calculated) {
        fprintf(stderr,"Error: SASA calculation has not been performed, "
                "no total SASA value available.\n");
        return -1.0;
    }
    return s->result->total;
}
double sasalib_area_class(const sasalib_t* s, sasalib_class c)
{    
    if (! s->calculated) {
        fprintf(stderr,"Error: SASA calculation has not been performed, "
                "no SASA value available. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    if (c < SASALIB_POLAR || c > SASALIB_CLASS_UNKNOWN) {
	fprintf(stderr,"Error: Requested SASA for illegal class of atoms "
		"in function sasalib_area_class(2).\n"
		"       Valid value are SASALIB_POLAR, SASALIB_APOLAR, "
		"SASALIB_NUCLEICACID and SASALIB_UNKNOWN\n");
	exit(EXIT_FAILURE);
    }
    return s->result->class[c];
}

const double* sasalib_area_atom_array(const sasalib_t *s)
{
    if (! s->calculated) {
        fprintf(stderr,"Error: SASA calculation has not been performed, "
                "no atomic SASA values are available.\n");
        return NULL;
    }
    return s->result->sasa;
}
double sasalib_radius_atom(const sasalib_t *s, int i)
{
    if (i >= 0 && i < s->n_atoms) {
        return s->r[i];
    }
    fprintf(stderr,"Error: Atom index %d invalid.\n",i);
    exit(EXIT_FAILURE);
}

void sasalib_set_proteinname(sasalib_t *s,const char *name)
{
    int n;
    if ((n = strlen(name)) > SASALIB_NAME_LIMIT) {
        strcpy(s->proteinname,"...");
        sprintf(s->proteinname+3,"%.*s",SASALIB_NAME_LIMIT-3,
                name+n+3-SASALIB_NAME_LIMIT);
    } else {
        strcpy(s->proteinname,name);
    }
}

const char* sasalib_get_proteinname(const sasalib_t *s) 
{
    return s->proteinname;
}

int sasalib_log(FILE *log, const sasalib_t *s)
{
    fprintf(log,"# Using van der Waals radii and atom classes defined \n"
            "# by Ooi et al (PNAS 1987, 84:3086-3090) and a probe radius\n"
            "# of %f Å.\n\n", SASA_PROBE_RADIUS);
    fprintf(log,"name: %s\n",s->proteinname);
    fprintf(log,"algorithm: %s\n",sasalib_alg_names[s->alg]);
#ifdef PTHREADS
    fprintf(log,"n_thread: %d\n",s->n_threads);
#endif
    
    switch(s->alg) {
    case SASALIB_SHRAKE_RUPLEY:
        fprintf(log,"n_testpoint: %d\n",s->n_sr);
        break;
    case SASALIB_LEE_RICHARDS:
        fprintf(log,"d_slice: %f Å\n",s->d_lr);
        break;
    default:
        fprintf(log,"Error: no SASA algorithm specified.\n");
        return 1;
    }
    fprintf(log,"time_elapsed: %f s\n",s->elapsed_time);
    fprintf(log,"n_atoms: %d\n", s->n_atoms);
    return 0;
}

void sasalib_per_residue(FILE *output, const sasalib_t *s)
{
    /*
    double sasa = 0;
    char buf[NBUF] = "", prev_buf[NBUF] = "";
    for (int i = 0; i < structure_n(s->p); ++i) {
        sprintf(buf,"%c_%d_%s",structure_atom_chain(s->p,i),
                atoi(structure_atom_res_number(s->p,i)),
                structure_atom_res_name(s->p,i));
        sasa += s->sasa[i];
        if (strcmp(buf,prev_buf)) {
            fprintf(output,"%s %f\n",prev_buf,sasa);
            strcpy(prev_buf,buf);
            sasa = 0;
        }
    }
    fprintf(output,"%s %f\n",buf,sasa);
    */
}

