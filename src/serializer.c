/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * serializer.c - Triples from photos
 *
 * Copyright (C) 2007-2008, David Beckett http://www.dajobe.org/
 * 
 * This file is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 *
 * USAGE: flickrdf [OPTIONS] FLICKR-PHOTO-URI
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <win32_flickcurl_config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <flickcurl.h>
#include <flickcurl_internal.h>


#define DC_NS "http://purl.org/dc/elements/1.1/"
#define GEO_NS "http://www.w3.org/2003/01/geo/wgs84_pos#"
#define FOAF_NS "http://xmlns.com/foaf/0.1/#"
#define XSD_NS "http://www.w3.org/2001/XMLSchema#"
#define RDF_NS "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

struct flickrdf_nspace_s
{
  char* prefix;
  char* uri;
  size_t prefix_len;
  size_t uri_len;
  int seen;
  struct flickrdf_nspace_s* next;
};
typedef struct flickrdf_nspace_s flickrdf_nspace;

flickrdf_nspace namespace_table[]={
  { (char*)"a",        (char*)"http://www.w3.org/2000/10/annotation-ns" },
  { (char*)"acl",      (char*)"http://www.w3.org/2001/02/acls#" },
  { (char*)"blue",     (char*)"http://machinetags.org/wiki/Blue#", },
  { (char*)"cell",     (char*)"http://machinetags.org/wiki/Cell#", },
  { (char*)"dc",       (char*)DC_NS },
  { (char*)"exif",     (char*)"http://nwalsh.com/rdf/exif#" },
  { (char*)"exifi",    (char*)"http://nwalsh.com/rdf/exif-intrinsic#" },
  { (char*)"flickr",   (char*)"http://machinetags.org/wiki/Flickr#" },
  { (char*)"filtr",    (char*)"http://machinetags.org/wiki/Filtr#", },
  { (char*)"foaf",     (char*)FOAF_NS },
  { (char*)"geo",      (char*)GEO_NS, },
  { (char*)"i",        (char*)"http://www.w3.org/2004/02/image-regions#" },
  { (char*)"ph"      , (char*)"http://machinetags.org/wiki/Ph#" },
  { (char*)"rdf",      (char*)RDF_NS },
  { (char*)"rdfs",     (char*)"http://www.w3.org/2000/01/rdf-schema#" },
  { (char*)"skos",     (char*)"http://www.w3.org/2004/02/skos/core" },
  { (char*)"upcoming", (char*)"http://machinetags.org/wiki/Upcoming#" },
  { (char*)"xsd",      (char*)XSD_NS, },
  { NULL, NULL }
};

#define FIELD_FLAGS_PERSON 1
#define FIELD_FLAGS_STRING 2

static struct {
  flickcurl_photo_field_type field;
  const char* nspace_uri;
  const char* name;
  int flags;
} field_table[]={
  /* dc:available -- date that the resource will become/did become available.*/
  /* dc:dateSubmitted - Date of submission of resource (e.g. thesis, articles)*/
  { PHOTO_FIELD_dateuploaded,       DC_NS,   "dateSubmitted" },
  { PHOTO_FIELD_license,            DC_NS,   "rights" },
  /* dc:modified - date on which the resource was changed. */
  { PHOTO_FIELD_dates_lastupdate,   DC_NS,   "modified" },
  /* dc:issued - date of formal issuance (e.g. publication of the resource */
  { PHOTO_FIELD_dates_posted,       DC_NS,   "issued" },
  /* dc:created - date of creation of the resource */
  { PHOTO_FIELD_dates_taken,        DC_NS,   "created" },
  { PHOTO_FIELD_description,        DC_NS,   "description" },
  { PHOTO_FIELD_location_latitude,  GEO_NS,  "lat", FIELD_FLAGS_STRING },
  { PHOTO_FIELD_location_longitude, GEO_NS,  "long", FIELD_FLAGS_STRING },
  { PHOTO_FIELD_owner_realname,     FOAF_NS, "name", FIELD_FLAGS_PERSON },
  { PHOTO_FIELD_owner_username,     FOAF_NS, "nick", FIELD_FLAGS_PERSON },
  { PHOTO_FIELD_title,              DC_NS,   "title" },
  { PHOTO_FIELD_none, NULL, NULL }
};


void
flickcurl_serializer_init(void)
{
  int i;

  for(i=0; namespace_table[i].prefix != NULL; i++) {
    namespace_table[i].uri_len=strlen(namespace_table[i].uri);
    namespace_table[i].prefix_len=strlen(namespace_table[i].prefix);
  }
}


void
flickcurl_serializer_terminate(void)
{
}



flickcurl_serializer*
flickcurl_new_serializer(flickcurl* fc, 
                         void* data, flickcurl_serializer_factory* factory)
{
  flickcurl_serializer* serializer;
  serializer=(flickcurl_serializer*)malloc(sizeof(flickcurl_serializer));
  if(!serializer)
    return NULL;
  
  serializer->fc=fc;
  serializer->data=data;
  serializer->factory=factory;
  return serializer;
}

void
flickcurl_free_serializer(flickcurl_serializer* serializer)
{
  free(serializer);
}


static flickrdf_nspace*
nspace_add_new(flickrdf_nspace* list, char* prefix, char *uri)
{
  flickrdf_nspace* ns;

  ns=(flickrdf_nspace*)malloc(sizeof(flickrdf_nspace));
  ns->prefix_len=strlen(prefix);
  ns->uri_len=strlen(uri);

  ns->prefix=(char*)malloc(ns->prefix_len+1);
  strcpy(ns->prefix, prefix);

  ns->uri=(char*)malloc(ns->uri_len+1);
  strcpy(ns->uri, uri);

  ns->next=list;
  return ns;
}


static flickrdf_nspace*
nspace_add_if_not_declared(flickrdf_nspace* list, 
                           const char* prefix, const char* nspace_uri)
{
  int n;
  flickrdf_nspace* ns;
  size_t prefix_len=prefix ? strlen(prefix) : 0;
  size_t uri_len=nspace_uri ? strlen(nspace_uri) : 0;
  
  for(ns=list; ns; ns=ns->next) {
    if(nspace_uri && ns->uri_len == uri_len && !strcmp(ns->uri, nspace_uri))
      break;
    if(prefix && ns->prefix_len == prefix_len && !strcmp(ns->prefix, prefix))
      break;
  }
  if(ns)
    return list;

  ns=NULL;
  for(n=0; namespace_table[n].uri; n++) {
    if(prefix && namespace_table[n].prefix_len == prefix_len && 
       !strcmp(namespace_table[n].prefix, prefix)) {
      ns=&namespace_table[n];
      break;
    }
    if(nspace_uri && namespace_table[n].uri_len == uri_len && 
       !strcmp(namespace_table[n].uri, nspace_uri)) {
      ns=&namespace_table[n];
      break;
    }
  }
  if(!ns)
    return list;

  /* ns was not found, copy it and add it to the list */
  return nspace_add_new(list, ns->prefix, ns->uri);
}


static flickrdf_nspace*
nspace_get_by_prefix(flickrdf_nspace* list, const char *prefix)
{
  flickrdf_nspace* ns;
  size_t prefix_len=strlen(prefix);
  
  for(ns=list; ns; ns=ns->next) {
    if(ns->prefix_len == prefix_len && !strcmp(ns->prefix, prefix))
      break;
  }
  return ns;
}


static void
print_nspaces(FILE* fh, const char* label, flickrdf_nspace* list)
{
  flickrdf_nspace* ns;

  for(ns=list; ns; ns=ns->next) {
    fprintf(fh, "%s: Declaring namespace prefix %s URI %s\n",
            label, (ns->prefix ? ns->prefix : ":"),
            (ns->uri ? ns->uri : "\"\""));

  }
}
    

static void
free_nspaces(flickrdf_nspace* list)
{
  flickrdf_nspace* next;

  for(; list; list=next) {
    next=list->next;
    if(list->prefix)
      free(list->prefix);
    free(list->uri);
    free(list);
  }
}
    

int
flickcurl_serialize_photo(flickcurl_serializer* fcs, flickcurl_photo* photo)
{
  int i;
  int need_person=0;
  flickrdf_nspace* nspaces=NULL;
  flickrdf_nspace* ns;
  flickcurl_serializer_factory* fsf=fcs->factory;
  flickcurl* fc=fcs->fc;
#ifdef FLICKCURL_DEBUG
  FILE* fh=stderr;
  const char* label="libflickcurl";
#endif

  if(!photo)
    return 1;
  
  /* Always add XSD */
  nspaces=nspace_add_if_not_declared(nspaces, NULL, XSD_NS);

  /* mark namespaces used in fields */
  for(i=PHOTO_FIELD_FIRST; i <= PHOTO_FIELD_LAST; i++) {
    flickcurl_photo_field_type field=(flickcurl_photo_field_type)i;
    flickcurl_field_value_type datatype=photo->fields[field].type;
    int f;

    if(datatype == VALUE_TYPE_NONE)
      continue;

    for(f=0; field_table[f].field != PHOTO_FIELD_none; f++) {
      if(field_table[f].field != field) 
        continue;

      if(field_table[f].flags & FIELD_FLAGS_PERSON)
        need_person=1;

      nspaces=nspace_add_if_not_declared(nspaces, NULL, field_table[f].nspace_uri);
      break;
    }

  }
  

  /* in tags look for xmlns:PREFIX="URI" otherwise look for PREFIX: */
  for(i=0; i < photo->tags_count; i++) {
    char* prefix;
    char *p;
    flickcurl_tag* tag=photo->tags[i];

    if(!strncmp(tag->raw, "xmlns:", 6)) {
      prefix=&tag->raw[6];
      for(p=prefix; *p && *p != '='; p++)
        ;
      if(!*p) /* "xmlns:PREFIX" seen */
        continue;

      /* "xmlns:PREFIX=" seen */
      *p='\0';
      nspaces=nspace_add_new(nspaces, prefix, p+1);
#ifdef FLICKCURL_DEBUG
        fprintf(fh,
                "%s: Found declaration of namespace prefix %s uri %s in tag '%s'\n",
                label, prefix, p+1, tag->raw);
#endif
      *p='=';
      continue;
    }

    prefix=tag->raw;
    for(p=prefix; *p && *p != ':'; p++)
      ;
    if(!*p) /* "PREFIX:" seen */
      continue;

    *p='\0';
    nspaces=nspace_add_if_not_declared(nspaces, prefix, NULL);
    *p=':';
  }


#ifdef FLICKCURL_DEBUG
    print_nspaces(fh, label , nspaces);
#endif

  /* generate seen namespace declarations */
  for(ns=nspaces; ns; ns=ns->next)
    fsf->emit_namespace(fcs->data,
                        ns->prefix, ns->prefix_len, ns->uri, ns->uri_len);
  
  if(need_person) {
    fsf->emit_triple(fcs->data,
                     photo->uri, FLICKCURL_TERM_TYPE_RESOURCE,
                     DC_NS, "creator",
                     "person", FLICKCURL_TERM_TYPE_ANONYMOUS,
                     NULL);
    fsf->emit_triple(fcs->data,
                     "person", FLICKCURL_TERM_TYPE_ANONYMOUS,
                     RDF_NS, "type",
                     FOAF_NS "Person", FLICKCURL_TERM_TYPE_RESOURCE,
                     NULL);
    fsf->emit_triple(fcs->data,
                     "person", FLICKCURL_TERM_TYPE_ANONYMOUS,
                     FOAF_NS, "maker",
                     photo->uri, FLICKCURL_TERM_TYPE_RESOURCE,
                     NULL);
  }
  

  /* generate triples from fields */
  for(i=PHOTO_FIELD_FIRST; i <= PHOTO_FIELD_LAST; i++) {
    flickcurl_photo_field_type field=(flickcurl_photo_field_type)i;
    flickcurl_field_value_type datatype=photo->fields[field].type;
    int f;

    if(datatype == VALUE_TYPE_NONE)
      continue;

    for(f=0; field_table[f].field != PHOTO_FIELD_none; f++) {
      const char* datatype_uri=NULL;
      char* object=NULL;
      int type= FLICKCURL_TERM_TYPE_LITERAL;
      
      if(field_table[f].field != field) 
        continue;

#ifdef FLICKCURL_DEBUG
        fprintf(fh,
                "%s: field %s (%d) with %s value: '%s' has predicate %s%s\n", 
                label,
                flickcurl_get_photo_field_label(field), field,
                flickcurl_get_field_value_type_label(datatype),
                photo->fields[field].string,
                field_table[f].nspace_uri, field_table[f].name);
#endif

      object=photo->fields[field].string;

      if(field_table[f].flags & FIELD_FLAGS_STRING)
        datatype=VALUE_TYPE_STRING;

      if(field == PHOTO_FIELD_license) {
        flickcurl_license* license;
        license=flickcurl_photos_licenses_getInfo_by_id(fc, 
                                                        photo->fields[field].integer);
        if(!license)
          continue;

        if(license->url) {
          datatype=VALUE_TYPE_URI;
          object=license->url;
        } else {
          datatype=VALUE_TYPE_STRING;
          object=license->name;
        }
      }
      
      switch(datatype) {
        case VALUE_TYPE_BOOLEAN:
          datatype_uri= XSD_NS "boolean";
          break;

        case VALUE_TYPE_DATETIME:
          datatype_uri= XSD_NS "dateTime";
          break;
          
        case VALUE_TYPE_FLOAT:
          datatype_uri= XSD_NS "double";
          break;
          
        case VALUE_TYPE_INTEGER:
          datatype_uri= XSD_NS "integer";
          break;
          
        case VALUE_TYPE_STRING:
          break;
          
        case VALUE_TYPE_URI:
          type= FLICKCURL_TERM_TYPE_RESOURCE;
          break;
          
        case VALUE_TYPE_NONE:
        case VALUE_TYPE_PHOTO_ID:
        case VALUE_TYPE_PHOTO_URI:
        case VALUE_TYPE_UNIXTIME:
        case VALUE_TYPE_PERSON_ID:
        case VALUE_TYPE_MEDIA_TYPE:
        default:
          break;
      }

      if(field_table[f].flags & FIELD_FLAGS_PERSON)
        fsf->emit_triple(fcs->data,
                         "person", FLICKCURL_TERM_TYPE_ANONYMOUS,
                         field_table[f].nspace_uri, field_table[f].name,
                         object, type,
                         datatype_uri);
      else
        fsf->emit_triple(fcs->data,
                         photo->uri, FLICKCURL_TERM_TYPE_RESOURCE,
                         field_table[f].nspace_uri, field_table[f].name,
                         object, type,
                         datatype_uri);
      break;
    }
  }
  

  /* generate triples from tags */
  for(i=0; i < photo->tags_count; i++) {
    flickcurl_tag* tag=photo->tags[i];
    char* prefix;
    char *p;
    char *f;
    char *v;
    size_t value_len;
    
    prefix=&tag->raw[0];

    if(!strncmp(prefix, "xmlns:", 6))
      continue;
    
    for(p=prefix; *p && *p != ':'; p++)
      ;
    /* ":" seen */
    *p='\0';

    f=p+1;
    
    for(v=f; *v && *v != '='; v++)
      ;
    if(!*v) /* "prefix:name" seen with no value */
      continue;
    /* zap = */
    *v++='\0';

    value_len=strlen(v);
    if(*v == '"') {
      v++;
      if(v[value_len-1]=='"')
        v[--value_len]='\0';
    }
        
    ns=nspace_get_by_prefix(nspaces, prefix);

#ifdef FLICKCURL_DEBUG
      fprintf(fh,
              "%s: prefix '%s' field '%s' value '%s' namespace uri %s\n",
              label, p, f, v, 
              ns->uri);
#endif
    
    fsf->emit_triple(fcs->data,
                     photo->uri, FLICKCURL_TERM_TYPE_RESOURCE,
                     ns->uri, f,
                     v, FLICKCURL_TERM_TYPE_LITERAL, 
                     NULL);
    
  }
  
  if(nspaces)
    free_nspaces(nspaces);
  
  flickcurl_free_photo(photo);

  if(fsf->emit_finish)
    fsf->emit_finish(fcs);
  
  return 0;
}
