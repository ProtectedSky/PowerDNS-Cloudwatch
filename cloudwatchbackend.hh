/*
    Cloudwatch Backend for PowerDNS
    Copyright (C) 2016 Protected Sky Pty Ltd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as published 
    by the Free Software Foundation

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PDNS_CWBACKEND_HH
#define PDNS_CWBACKEND_HH

#include "pdns/pdnsexception.hh"
#include "pdns/logger.hh"
#include "pdns/arguments.hh"
#include "pdns/statbag.hh"
#include <sys/syscall.h>
#include <unistd.h>
#include <algorithm>
#include <sys/time.h>
#include <sys/prctl.h>


#include <aws/core/Aws.h>
#include <aws/core/Version.h>
#include <aws/monitoring/CloudWatchClient.h>
#include <aws/monitoring/model/PutMetricDataRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#define LOGPREFIX "[cloudwatch] "

extern StatBag S;

class CloudWatchSender {
    private:
        vector<string> metricsToSend;
        vector<string> metricsToZero;
        vector<string> metricDimensions;
        bool isFuncStat(string metric);
        std::shared_ptr<Aws::CloudWatch::CloudWatchClient> cw;
        Aws::SDKOptions options;
        Aws::Client::ClientConfiguration config;
        Aws::Vector<Aws::CloudWatch::Model::Dimension> getDimensions();
    public:
        CloudWatchSender();
        ~CloudWatchSender();
        bool sendStatistics();
        static void* threadRunner(void*);
        static void declareArguments();
};

class CloudWatchLoader {
    public:
        CloudWatchLoader();
};

static CloudWatchLoader loader;  

#endif //PDNS_CWBACKEND_HH
