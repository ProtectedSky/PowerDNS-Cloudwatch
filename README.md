# PowerDNS Cloudwatch Module
C++ Module for PowerDNS to send statistics to Amazon Web Service's (AWS) CloudWatch

Tested on v3.4.7 (latest on Debian Jessie)

We use this in production daily to scale our servers during peak times (see the graph below)

## Example from Cloudwatch
![Cloudwatch Example](http://psky.me/cw.png)

## Compiling
```
# TODO
```

## Using the module
1. Install module into PowerDNS' module-dir (Debian x86_64 default is /usr/lib/x86_64-linux-gnu/pdns)
2. Load the module from your config
```
load-modules=cloudwatch
#or you can specify a path
#load-modules=/path/to/libcloudwatchbackend.so
```
3. Configure Cloudwatch
```
cloudwatch-namespace=PSKY/RBL
cloudwatch-dimensions=RBL/Production,AutoScalingGroup/PSKYPRODASG
cloudwatch-aws-access-key-id=IAM-CREDENTIALS-HERE
cloudwatch-aws-secret-access-key=IAM-CREDENTIALS-HERE
cloudwatch-metrics=udp-queries
cloudwatch-aws-region=us-west-2
cloudwatch-interval=60
```
## FAQ

### What does this do?
This integrates with PowerDNS to send statistics to AWS from PowerDNS itself as a seperate thread.

This is similar to the [built in Carbon graphing functionality](https://blog.powerdns.com/2014/12/11/powerdns-graphing-as-a-service/) provided by PowerDNS.

### Why a module and not a script?
By building this natively into PowerDNS we reduce delay and overhead from constantly polling PowerDNS
We also get gaps in metrics if PowerDNS itself crashes and a Zero metric upon startup, allowing us to monitor PowerDNS for crashes via CloudWatch

### Why would I want this?
Cloudwatch Alarms can trigger actions such as Scaling an AutoScalingGroup. [See AWS' doco for more info](http://docs.aws.amazon.com/autoscaling/latest/userguide/policy_creating.html)

### What does this require?
This requires you to have the AWS C++ SDK already installed on your system, and Knowledge of building PowerDNS modules.

***If there is enough demand we will consider making a repo for precompiled modules.***
