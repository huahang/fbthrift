/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thrift/lib/cpp/util/kerberos/Krb5OlderVersionStubs.h"

#include <assert.h>
#include <glog/logging.h>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

int gss_krb5_import_cred(
    int *minor_status,
    krb5_ccache id,
    krb5_principal keytab_principal,
    krb5_keytab keytab,
    gss_cred_id_t *cred) {
  LOG(ERROR) << "Linking against older version of krb5 which does not support "
             << "gss_krb5_import_cred";
  return GSS_S_NO_CRED;
}

krb5_boolean krb5_is_config_principal(
    krb5_context context, krb5_const_principal principal) {
  LOG(ERROR) << "Linking against older version of krb5 which does not support "
             << "krb5_is_config_principal";
  return false;
}