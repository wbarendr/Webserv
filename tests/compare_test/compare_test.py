#!/usr/bin/python

import sys,subprocess,os,linecache as lc

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

webservPort = "5000"
nginxPort= "8080"
ret = 0

def runPort(port, file, method, headers, body, url_file_name, print_out):
    # url_file_name = "/put_test/101" + port + ".txt"
    if method == "PUT":
        url_file_name += port
    host = "localhost:"+port+url_file_name
    file_keys = "\"sleutel=hallo webserver\""
    args = []
    args.append("curl")
    args.append("-i")
    args.append("-X")
    args.append(method)
    # args.append(auth)
    for i in headers:
        args.append("-H")
        args.append(i)
    if method == "PUT":
        args.append("-F")
        args.append(file_keys)
    args.append(host)
    # print("hello")
    # print(args)
    output = subprocess.run(args, capture_output=True)
    if print_out == 1:
        print (output.stdout.decode())
    else:
        file.write(output.stdout.decode())
    # file.close()

def runtester():
    webservFile = open("webservFile", "w+")
    webservFile.truncate(0)
    output = subprocess.run(["curl", "-i" ,"-X", "PUT",  "http://localhost:5000/put_test/chicken.png", "--upload-file", "tests/chicken.png"], capture_output=True)
    webservFile.write(output.stdout.decode())
    webservFile.close()
    statusWeb = lc.getline("webservFile", 1)
    lc.clearcache()
    status_code_Web = statusWeb[statusWeb.find(' ')+1:12]
    print (bcolors.OKGREEN + status_code_Web + bcolors.ENDC)
    webservFile.close()
    os.remove("webservFile")
    return status_code_Web

def runCommand(port):
    print(bcolors.WARNING + "-----------------------------------------------"+ bcolors.ENDC)
    if runBoth("GET", ["User-agent: Go-http-client/1.1", "Accept-Encoding: gzip"], "", "/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: de-DE"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Charset: iso-8859-15"], "", "/charset/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Charset: utf-8"], "", "/charset/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Charset: *"], "", "/charset/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: es-SP"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: fr-FR, tu-TU, nl-NL"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: nl;q=-ru"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: es-ES, nl;q=-ru"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("GET", ["Accept-Language: *"], "", "/language/", port) != "200":
        sys.exit(5)
    if runBoth("POST", ["User-agent: Go-http-client/1.1", "Accept-Encoding: chunked", "Accept-Type: test/file"], "", "/", port) != "405":
        sys.exit(5)
    tmp_var = runBoth("PUT", ["'AUTHORIZATION: Basic d2Vic2VydjpjaGVlc2U='", "Connection: keep-alive"], "", 
            "/put_test/111.txt", port)
    if (tmp_var != "201" and tmp_var != "204"):
        sys.exit(5)
    tmp_var = runBoth("PUT", ["'AUTHORIZATION: Basic d2Vic2VydjpjaGVlc2U='", "Connection: keep-alive", 
            "X-secrets: chips", "X-other_secret: cake", "X-final_secret: maple syrup", "-X-Not-a-Secret: nothing"], "", 
            "/put_test/111.txt", port)
    if tmp_var != "201" and tmp_var != "204":
        sys.exit(5)
    tmp_var = runtester()
    if tmp_var != "201" and tmp_var != "204":
        sys.exit(5)

    print(bcolors.WARNING + "-----------------------------------------------"+ bcolors.ENDC)


def runBoth(method, headers, body, url, port):#webservPort, nginxPort):
    webservFile = open("webservFile", "w+")
    webservFile.truncate(0)
    nginxFile = open("nginxFile", "w+")
    nginxFile.truncate(0)
    args = []
    if port == "0":
        args2 = []
        args.append("Host: localhost:"+"8080")
        args2.append("Host: localhost:"+"5000")
        for i in headers:
            args.append(i)
            args2.append(i)
        runPort("8080", nginxFile, method, args, body, url, 0)
        runPort("5000", webservFile, method, args2, body, url, 0)
        nginxFile.close()
        webservFile.close()
        # os.system("diff webservFile nginxFile")
        # webservFile.truncate(0)
        # nginxFile.truncate(0)
        # for line in webservFile:
        #     print(line)
        statusWeb = lc.getline("webservFile", 1)
        lc.clearcache()
        status_code_Web = statusWeb[statusWeb.find(' ')+1:12]
        # print(bcolors.WARNING + statusWeb + bcolors.ENDC)
        statusNginx = lc.getline("nginxFile", 1)
        lc.clearcache()
        status_code_Nginx = statusNginx[statusNginx.find(' ')+1:12]
        if status_code_Web == status_code_Nginx:
            print(bcolors.OKGREEN + "SAME STATUS CODE" + bcolors.ENDC)
        else:
            print(bcolors.FAIL + statusWeb + bcolors.ENDC)
            print(bcolors.FAIL + statusNginx + bcolors.ENDC)
            global ret
            ret += 1
        # nginxFile = open("nginxFile", "w+")
        # nginxFile.truncate(0)
        # nginxFile.close()
        # webservFile = open("webservFile", "w+")
        # webservFile.truncate(0)
        # webservFile.close()
    else:
        args.append("Host: localhost:"+port)
        for i in headers:
            args.append(i)
        if port == "8080":
            runPort(port, nginxFile, method, args, body, url, 1)
        elif port == "5000":
            runPort(port, webservFile, method, args, body, url, 0)
            webservFile.close()
            statusWeb = lc.getline("webservFile", 1)
            lc.clearcache()
            status_code_Web = statusWeb[statusWeb.find(' ')+1:12]
            print (bcolors.OKGREEN + status_code_Web + bcolors.ENDC)
            nginxFile.close()
            webservFile.close()
            os.remove("nginxFile")
            os.remove("webservFile")
            return status_code_Web
    nginxFile.close()
    webservFile.close()
    os.remove("nginxFile")
    os.remove("webservFile")



print ("Make sure webserv is running on", webservPort, "and nginx on", nginxPort)

if len(sys.argv) != 2:
    print ('Please give 1 argument')
    exit()

if sys.argv[1] == 'nginx':
    runCommand(nginxPort)
elif sys.argv[1] == 'webserv':
    runCommand(webservPort)
elif sys.argv[1] == 'compare':
    runCommand("0")
else :
    print ("NOT RECOGNIZED - first argument should be: nginx, webserv or compare")
# print (ret)
sys.exit(ret)
    
