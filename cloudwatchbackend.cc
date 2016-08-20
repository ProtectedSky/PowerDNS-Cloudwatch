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
#include "cloudwatchbackend.hh"

CloudWatchSender::CloudWatchSender() {
    stringtok(metricsToSend, arg()["cloudwatch-metrics"], ", ");
    stringtok(metricsToZero, arg()["cloudwatch-metrics-zero"], ", ");
    stringtok(metricDimensions, arg()["cloudwatch-dimensions"], ", ");

    L << Logger::Warning << LOGPREFIX << "reporting " << metricsToSend.size() << " metric(s) to " << arg()["cloudwatch-aws-region"] << endl;
    Aws::InitAPI(options); 
    config.region = Aws::RegionMapper::GetRegionFromName(arg()["cloudwatch-aws-region"].c_str());
    if(arg()["cloudwatch-aws-access-key-id"].length()==0 || arg()["cloudwatch-aws-secret-access-key"].length()==0) {
        throw PDNSException("Invalid AWS Credentials");
    }
    cw = Aws::MakeShared<Aws::CloudWatch::CloudWatchClient>("CloudWatchClient",Aws::Auth::AWSCredentials(arg()["cloudwatch-aws-access-key-id"].c_str(), arg()["cloudwatch-aws-secret-access-key"].c_str()), config);
}

CloudWatchSender::~CloudWatchSender() {
    Aws::ShutdownAPI(options);
}

bool CloudWatchSender::isFuncStat(string metric) {
    return (S.getPointer(metric)==NULL);
}

Aws::Vector<Aws::CloudWatch::Model::Dimension> CloudWatchSender::getDimensions() {
    Aws::Vector<Aws::CloudWatch::Model::Dimension> dimensions;
    for(const string& dimension : metricDimensions) {
        vector<string> dimensionality;
        stringtok(dimensionality, dimension, "/");
        if(dimensionality.size() < 2) {
            continue;
        }
        Aws::CloudWatch::Model::Dimension d;
        d.SetName(dimensionality[0].c_str());
        d.SetValue(dimensionality[1].c_str());
        dimensions.push_back(d);
    }
    return dimensions;
}

bool CloudWatchSender::sendStatistics() {
    L << Logger::Debug << LOGPREFIX << "CloudWatch Update!" << endl;
    vector<string> metricsAvailable = S.getEntries();
    Aws::Vector<Aws::CloudWatch::Model::MetricDatum> cloudwatchDatum;

    for(const string& metric : metricsToSend) {
         if(std::find(metricsAvailable.begin(), metricsAvailable.end(), metric) != metricsAvailable.end()) {
            L << Logger::Debug << LOGPREFIX << metric << ": " << S.read(metric) << " " << isFuncStat(metric) << endl;
             Aws::CloudWatch::Model::MetricDatum thisMetric;
             thisMetric.SetMetricName(metric.c_str());
             thisMetric.SetValue(S.read(metric));
             thisMetric.SetDimensions(getDimensions());
             //see if we need to zero the metric (if non function + in our zero list + > 0 )
             if(!isFuncStat(metric) && std::find(metricsToZero.begin(), metricsToZero.end(), metric) != metricsToZero.end() && S.read(metric) > 0) {
                L << Logger::Debug << LOGPREFIX << "Zeroing: " << metric << endl;
                S.set(metric, 0);
             }
             cloudwatchDatum.push_back(thisMetric);
         }
    }
    
    Aws::CloudWatch::Model::PutMetricDataRequest m;
    m.SetNamespace(arg()["cloudwatch-namespace"].c_str());
    m.SetMetricData(cloudwatchDatum);
    Aws::CloudWatch::Model::PutMetricDataOutcome outcome = cw->PutMetricData(m);
    //TODO handle throttling from AWS + 'timed out' errors
    if(!outcome.IsSuccess()) {
        L << Logger::Error << LOGPREFIX << "Error from CloudWatch: " << outcome.GetError().GetMessage().c_str() << endl;
        return false;
    }
    return true;
}

void* CloudWatchSender::threadRunner(void*) {
    try {
        //for identification within htop
        prctl(PR_SET_NAME,"pdns cloudwatch\0",NULL,NULL,NULL);
        CloudWatchSender c;
        for(;;) {
            c.sendStatistics();
            //try to synchronise the sending of metrics across nodes (assuming NTP)
            int sleepTime = arg().asNum("cloudwatch-interval");
            sleepTime = sleepTime-time(0)%sleepTime;
            L << Logger::Debug << LOGPREFIX << "sleeping for: " << sleepTime << endl;
            sleep(sleepTime);
        }
    }
    catch(std::exception& e) {
      L << Logger::Error << LOGPREFIX << "CloudWatch thread died: " << e.what() << endl;
      return 0;
    }
    catch(PDNSException& e) {
      L << Logger::Error << LOGPREFIX << "CloudWatch thread died, PDNSException: " << e.reason << endl;
      return 0;
    }
    catch(...) {
      L << Logger::Error << LOGPREFIX << "CloudWatch thread died" << endl;
      return 0;
    }
}

void CloudWatchSender::declareArguments() {
    arg().set("cloudwatch-dimensions","Comma seperated dimensions to send alongside the metrics eg n/v,n/v,n/v")="Server/PowerDNS";
    arg().set("cloudwatch-interval","Interval for the cloudwatch thread to send metrics to AWS")="60";
    arg().set("cloudwatch-aws-access-key-id","IAM Key Id to use for cloudwatch metrics")="";
    arg().set("cloudwatch-aws-secret-access-key","IAM Secret Key to use for cloudwatch metrics")="";
    arg().set("cloudwatch-aws-region","AWS Region")="us-west-2";
    arg().set("cloudwatch-metrics","Comma delimited names of metrics to send to cloudwatch")="udp-queries";
    arg().set("cloudwatch-metrics-zero","Comma delimited names of metrics to ZERO after sending to cloudwatch")="corrupt-packets,deferred-cache-inserts,deferred-cache-lookup,dnsupdate-answers,dnsupdate-changes,dnsupdate-queries,dnsupdate-refused,packetcache-hit,packetcache-miss,packetcache-size,query-cache-hit,query-cache-miss,rd-queries,recursing-answers,recursing-questions,recursion-unanswered,security-status,servfail-packets,signatures,tcp-answers,tcp-queries,timedout-packets,udp-answers,udp-answers-bytes,udp-do-queries,udp-queries,udp4-answers,udp4-queries,udp6-answers,udp6-queries,corrupt-packets,deferred-cache-inserts,deferred-cache-lookup,dnsupdate-answers,dnsupdate-changes,dnsupdate-queries,dnsupdate-refused,packetcache-hit,packetcache-miss,packetcache-size,query-cache-hit,query-cache-miss,rd-queries,recursing-answers,recursing-questions,recursion-unanswered,security-status,servfail-packets,signatures,tcp-answers,tcp-queries,timedout-packets,udp-answers,udp-answers-bytes,udp-do-queries,udp-queries,udp4-answers,udp4-queries,udp6-answers,udp6-queries,corrupt-packets,deferred-cache-inserts,deferred-cache-lookup,dnsupdate-answers,dnsupdate-changes,dnsupdate-queries,dnsupdate-refused,packetcache-hit,packetcache-miss,packetcache-size,query-cache-hit,query-cache-miss,rd-queries,recursing-answers,recursing-questions,recursion-unanswered,security-status,servfail-packets,signatures,tcp-answers,tcp-queries,timedout-packets,udp-answers,udp-answers-bytes,udp-do-queries,udp-queries,udp4-answers,udp4-queries,udp6-answers,udp6-queries";
    arg().set("cloudwatch-namespace","Cloudwatch Namespace to publish metrics to")="";
}

CloudWatchLoader::CloudWatchLoader() {
    CloudWatchSender::declareArguments();
    pthread_t qtid;
    pthread_create(&qtid,0,&CloudWatchSender::threadRunner, 0);
}
