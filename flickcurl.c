/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * flickcurl utility - Invoke the Flickrcurl library
 *
 * Copyright (C) 2007, David Beckett http://purl.org/net/dajobe/
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
 * USAGE: flickcurl [OPTIONS] flickr-api-command args...
 *
 * ~/.flickcurl.conf should contain the authentication details in the form:
 * [flickr]
 * auth_token=1234567-8901234567890123
 * api_key=0123456789abcdef0123456789abcdef
 * secret=fedcba9876543210
 *
 * To authenticate from a FROB - to generate an auth_token from a FROB use:
 *   flickcurl -a FROB
 * FROB like 123-456-789
 * which will write a new ~/.flickcurl.conf with the auth_token received
 *
 * Flickr API Calls:
 *
 * flickcurl test-echo KEY VALUE
 *   This method does not require authentication.
 * Echoes back the KEY and VALUE received - an API test.
 *
 * flickcurl photo-getinfo PHOTO-ID
 *   PHOTO-ID like 123456789
 *   This method does not require authentication.
 *   -- http://www.flickr.com/services/api/flickr.photos.getInfo.html
 * Gets information about a photo including its tags
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/* many places for getopt */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <flickcurl_getopt.h>
#endif

#include <flickcurl.h>



#ifdef NEED_OPTIND_DECLARATION
extern int optind;
extern char *optarg;
#endif


static const char* program;

static const char*
my_basename(const char *name)
{
  char *p;
  if((p=strrchr(name, '/')))
    name=p+1;
  else if((p=strrchr(name, '\\')))
    name=p+1;

  return name;
}


static void
my_message_handler(void *user_data, const char *message)
{
  fprintf(stderr, "%s: ERROR: %s\n", program, message);
}


static void
my_set_config_var_handler(void* userdata, const char* key, const char* value)
{
  flickcurl *fc=(flickcurl *)userdata;
  
  if(!strcmp(key, "api_key"))
    flickcurl_set_api_key(fc, value);
  else if(!strcmp(key, "secret"))
    flickcurl_set_shared_secret(fc, value);
  else if(!strcmp(key, "auth_token"))
    flickcurl_set_auth_token(fc, value);
}


#ifdef HAVE_GETOPT_LONG
#define HELP_TEXT(short, long, description) "  -" short ", --" long "  " description
#define HELP_TEXT_LONG(long, description) "      --" long "  " description
#define HELP_ARG(short, long) "--" #long
#define HELP_PAD "\n                          "
#else
#define HELP_TEXT(short, long, description) "  -" short "  " description
#define HELP_TEXT_LONG(long, description)
#define HELP_ARG(short, long) "-" #short
#define HELP_PAD "\n      "
#endif


#define GETOPT_STRING "a:d:hv"

#ifdef HAVE_GETOPT_LONG
static struct option long_options[] =
{
  /* name, has_arg, flag, val */
  {"auth",    1, 0, 'a'},
  {"delay",   1, 0, 'd'},
  {"help",    0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {NULL,      0, 0, 0}
};
#endif


typedef int (*command_handler)(flickcurl* fc, int argc, char *argv[]);


static int
command_test_echo(flickcurl* fc, int argc, char *argv[])
{
  return flickcurl_test_echo(fc, argv[1], argv[2]);
}


#if 0
static void command_flickcurl_tag_handler(void *user_data, flickcurl_tag* tag)
{
  fprintf(stderr, "%s: photo %s tag: id %s author %s raw '%s' cooked '%s'\n",
          program, tag->photo->id,
          tag->id, tag->author, tag->raw, tag->cooked);
}
#endif


static int
command_people_getInfo(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_person* person;

  person=flickcurl_people_getInfo(fc, argv[1]);

  if(person) {
    flickcurl_person_field field;
    
    fprintf(stderr, "Found person with ID %s\n", person->nsid);

    for(field=0; field <= PERSON_FIELD_LAST; field++) {
      flickcurl_field_value_type datatype=person->fields[field].type;

      if(datatype == VALUE_TYPE_NONE)
        continue;
      
      fprintf(stderr, "field %s (%d) with %s value: '%s' / %d\n", 
              flickcurl_get_person_field_label(field), field,
              flickcurl_get_field_value_type_label(datatype),
              person->fields[field].string, person->fields[field].integer);
    }
    
    flickcurl_free_person(person);
  }
  
  return (person != NULL);
}


static int
command_photos_getInfo(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_photo* photo;

#if 0
  flickcurl_set_tag_handler(fc, command_flickcurl_tag_handler, NULL);
#endif

  photo=flickcurl_photos_getInfo(fc, argv[1]);

  if(photo) {
    flickcurl_photo_field field;
    int i;
    
    fprintf(stderr, "%s: Found photo with URI %s ID %s and %d tags\n",
            program, photo->uri, photo->id, photo->tags_count);

    for(field=0; field <= PHOTO_FIELD_LAST; field++) {
      flickcurl_field_value_type datatype=photo->fields[field].type;

      if(datatype == VALUE_TYPE_NONE)
        continue;
      
      fprintf(stderr, "field %s (%d) with %s value: '%s' / %d\n", 
              flickcurl_get_photo_field_label(field), field,
              flickcurl_get_field_value_type_label(datatype),
              photo->fields[field].string, photo->fields[field].integer);
    }
    

    for(i=0; i < photo->tags_count; i++) {
      flickcurl_tag* tag=photo->tags[i];
      fprintf(stderr, "%d) %s tag: id %s author %s raw '%s' cooked '%s'\n",
              i, (tag->machine_tag ? "machine" : "regular"),
              tag->id, tag->author, tag->raw, tag->cooked);
    }
    flickcurl_free_photo(photo);
  }
  
  return (photo != NULL);
}


static int
command_photos_licenses_getInfo(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_license** licenses;
  int i;
  
  licenses=flickcurl_photos_licenses_getInfo(fc);
  if(licenses) {

    fprintf(stderr, "%s: Found licenses\n", program);

    for(i=0; licenses[i]; i++) {
      flickcurl_license* license=licenses[i];
      fprintf(stderr, "%d) license: id %d name '%s' url %s\n",
              i, license->id, license->name, 
              license->url ? license->url : "(none)");
      
    }
  }
  
  return (licenses != NULL);
}


static int
command_urls_lookupUser(flickcurl* fc, int argc, char *argv[])
{
  char* nsid=NULL;
  char* url=argv[1];
  
  nsid=flickcurl_urls_lookupUser(fc, url);

  if(nsid)
    fprintf(stderr, "%s: NSID %s for user profile/photo URL %s\n", 
            program, nsid, url);
  
  return (nsid != NULL);
}


static void
command_contexts_print(FILE* fh, flickcurl_context** contexts)
{
  flickcurl_context* context;
  int i;
  
  for(i=0; (context=contexts[i]); i++) {
    const char* label=flickcurl_get_context_type_field_label(context->type);
    fprintf(fh, "%d) context type '%s' id %s secret %s server %d farm %d\n  title: %s\n  url: %s\n  thumb: %s\n",
            i, label, 
            context->id,
            (context->secret ? context->secret : "NULL"),
            context->server, context->farm,
            (context->title ? context->title : "NULL"),
            (context->url ? context->url : "NULL"),
            (context->thumb ? context->thumb : "NULL")
            );
  }
}


static int
command_groups_pools_getContext(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_context** contexts;

  contexts=flickcurl_groups_pools_getContext(fc, argv[1], argv[2]);
  if(!contexts)
    return 1;
  fprintf(stderr, "%s: Pool context of photo %s in pool %s:\n", program,
          argv[1], argv[2]);
  command_contexts_print(stderr, contexts);
  
  flickcurl_free_contexts(contexts);

  return 0;
}

static int
command_photos_getAllContexts(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_context** contexts;
  
  contexts=flickcurl_photos_getAllContexts(fc, argv[1]);
  if(!contexts)
    return 1;
  fprintf(stderr, "%s: Photos %s all contexts:\n", program, argv[1]);
  command_contexts_print(stderr, contexts);
  
  flickcurl_free_contexts(contexts);

  return 0;
}

static int
command_photos_getContext(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_context** contexts;

  contexts=flickcurl_photos_getContext(fc, argv[1]);
  if(!contexts)
    return 1;
  fprintf(stderr, "%s: Photos %s context:\n", program, argv[1]);
  command_contexts_print(stderr, contexts);
  
  flickcurl_free_contexts(contexts);

  return 0;
}

static int
command_photosets_getContext(flickcurl* fc, int argc, char *argv[])
{
  flickcurl_context** contexts;

  contexts=flickcurl_photosets_getContext(fc, argv[1], argv[2]);
  if(!contexts)
    return 1;
  fprintf(stderr, "%s: Photo %s in photoset %s context:\n", program,
          argv[1], argv[2]);
  command_contexts_print(stderr, contexts);
  
  flickcurl_free_contexts(contexts);

  return 0;
}



static struct {
  const char*     name;
  const char*     args;
  const char*     description;
  command_handler handler;
  int             min;
  int             max;
} commands[] = {
  /* name, min, handler */
  {"groups-pools-getContext",
   "PHOTO-ID GROUP-ID", "Get next and previous photos for a photo in a group pool.",
   command_groups_pools_getContext, 2, 2},
  {"people-getInfo",
   "USER-ID", "Get information about one person with id USER-ID", 
   command_people_getInfo,  1, 1},
  {"photos-getContext",
   "PHOTO-ID", "Get next and previous photos for a photo in a photostream.",
   command_photos_getContext, 1, 1},
  {"photos-getAllContexts",
   "PHOTO-ID", "Get all visible sets and pools the photo belongs to.",
   command_photos_getAllContexts, 1, 1},
  {"photos-getInfo",
   "PHOTO-ID", "Get information about one photo with id PHOTO-ID", 
   command_photos_getInfo,  1, 1},
  {"photos-licenses-getInfo",
   "", "Get list of available photo licenses", 
   command_photos_licenses_getInfo,  0, 0},
  {"photosets-getContext",
   "PHOTO-ID PHOTOSET-ID", "Get next and previous photos for a photo in a set.",
   command_photosets_getContext, 2, 2},
  {"test-echo",
   "KEY VALUE", "Test echo of KEY VALUE",
   command_test_echo,  2, 2},
  {"urls-lookupUser",
   "URL", "Get a user NSID given the url to a user's photo", 
   command_urls_lookupUser,  1, 1},

  {NULL, NULL, NULL, 0, 0}
};  
  


static const char *title_format_string="Flickr API utility %s\n";

static const char* config_filename=".flickcurl.conf";
static const char* config_section="flickr";


int
main(int argc, char *argv[]) 
{
  flickcurl *fc;
  int rc=0;
  int usage=0;
  int help=0;
  int cmd_index= -1;
  int read_auth=1;
  int i;
  const char* home;
  char config_path[1024];
  int request_delay= -1;
  
  program=my_basename(argv[0]);

  home=getenv("HOME");
  if(home)
    sprintf(config_path, "%s/%s", home, config_filename);
  else
    strcpy(config_path, config_filename);
  

  while (!usage && !help)
  {
    int c;
    char* auth_token=NULL;
    
#ifdef HAVE_GETOPT_LONG
    int option_index = 0;

    c = getopt_long (argc, argv, GETOPT_STRING, long_options, &option_index);
#else
    c = getopt (argc, argv, GETOPT_STRING);
#endif
    if (c == -1)
      break;

    switch (c) {
      case 0:
      case '?': /* getopt() - unknown option */
        usage=1;
        break;

      case 'a':
        /* Exchange the mini-token for a full token */
        auth_token=flickcurl_auth_getFullToken(fc, optarg);
        if(!auth_token) {
          fprintf(stderr, 
                  "%s: Could not find auth_token in getFullToken response\n",
                  program);
        } else {
          FILE* fh;

          flickcurl_set_auth_token(fc, auth_token);
          
          fh=fopen(config_path, "w");
          if(!fh) {
            fprintf(stderr, "%s: Failed to write to config filename %s: %s\n",
                    program, config_path, strerror(errno));
          } else {
            fputs("[flickr]\n", fh);
            fprintf(fh,
                    "[flickr]\napi_key=%s\nsecret=%s\nauth_token=%s\n", 
                    flickcurl_get_api_key(fc),
                    flickcurl_get_shared_secret(fc),
                    flickcurl_get_auth_token(fc));
            fclose(fh);
            read_auth=0;
          }
        }
        
        break;

      case 'd':
        if(optarg)
          request_delay=atoi(optarg);
        break;
        
      case 'h':
        help=1;
        break;

      case 'v':
        fputs(flickcurl_version_string, stdout);
        fputc('\n', stdout);

        exit(0);
    }
    
  }

  argv+=optind;
  argc-=optind;
  
  if(!help && argc < 1)
    usage=2; /* Title and usage */

  if(!help && !argc) {
    fprintf(stderr, "%s: No command given\n", program);
    usage=1;
    goto usage;
  }

  if(usage || help)
    goto usage;


  /* Initialise the Flickcurl library */
  fc=flickcurl_new();
  if(!fc) {
    rc=1;
    goto tidy;
  }

  flickcurl_set_error_handler(fc, my_message_handler, NULL);

  if(read_auth && !access((const char*)config_path, R_OK)) {
    if(read_ini_config(config_path, config_section, fc,
                       my_set_config_var_handler)) {
      fprintf(stderr, "%s: Failed to read config filename %s: %s\n",
              program, config_path, strerror(errno));
      rc=1;
      goto tidy;
    }
  }

  if(request_delay >= 0)
    flickcurl_set_request_delay(fc, request_delay);
  
  
  for(i=0; commands[i].name; i++)
    if(!strcmp(argv[0], commands[i].name)) {
      cmd_index=i;
      break;
    }
  if(cmd_index < 0) {
    fprintf(stderr, "%s: No such command `%s'\n", program, argv[0]);
    usage=1;
    goto usage;
  }


 usage:
  if(usage) {
    if(usage>1) {
      fprintf(stderr, title_format_string, flickcurl_version_string);
      fputs("Flickcurl home page: ", stderr);
      fputs(flickcurl_home_url_string, stderr);
      fputc('\n', stderr);
      fputs(flickcurl_copyright_string, stderr);
      fputs("\nLicense: ", stderr);
      fputs(flickcurl_license_string, stderr);
      fputs("\n\n", stderr);
    }
    fprintf(stderr, "Try `%s " HELP_ARG(h, help) "' for more information.\n",
            program);
    exit(1);
  }

  if(help) {
    printf(title_format_string, flickcurl_version_string);
    puts("Call the Flickr API to get information.");
    printf("Usage: %s [OPTIONS] command args...\n\n", program);

    fputs(flickcurl_copyright_string, stdout);
    fputs("\nLicense: ", stdout);
    puts(flickcurl_license_string);
    fputs("Flickcurl home page: ", stdout);
    puts(flickcurl_home_url_string);

    fputs("\n", stdout);

    puts(HELP_TEXT("a", "auth FROB       ", "Authenticate with a FROB and write auth config"));
    puts(HELP_TEXT("d", "delay DELAY     ", "Set delay between requests in milliseconds"));
    puts(HELP_TEXT("h", "help            ", "Print this help, then exit"));
    puts(HELP_TEXT("v", "version         ", "Print the flickcurl version"));

    fputs("\nCommands\n", stdout);
    for(i=0; commands[i].name; i++)
      printf("  %-28s %s\n    %s\n", commands[i].name, commands[i].args,
             commands[i].description);

    exit(0);
  }


  /* Perform the API call */
  rc=commands[cmd_index].handler(fc, argc, argv);

 tidy:
  if(fc)
    flickcurl_free(fc);

  return(rc);
}