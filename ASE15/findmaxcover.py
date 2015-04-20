import sys
import subprocess
import glob

prefix = sys.argv[1]
harness = sys.argv[2]
mutant_base = sys.argv[3]
start = int(sys.argv[4])
covprefix = sys.argv[5]
canhitprefix = sys.argv[6]
options = ""
for o in sys.argv[7:]:
    options += o + " "

print prefix
print harness
print mutant_base
print start

mutStrength = {}

for m in glob.glob(covprefix + "_mutant*_" + mutant_base):
    print "Analyzing mutant",m
    sys.stdout.flush()
    sys.stderr.flush()
    t = start
    foundCover = False
    mhit = canhitprefix + "." + m
    mhit = mhit.replace(covprefix,"COVER")
    mhit = mhit + ".result"
    mutantOk = None
    for l in open (mhit):
        if "VERIFICATION FAILED" in l:
            mutantOk = True
            break
        if "VERIFICATION SUCCESSFUL" in l:
            print "Mutant can't be covered"
            mutStrength[m] = -1
            break
    if mutantOk == None:
        print "Mutant doesn't compile"
        mutStrength[m] = -2
    if not mutantOk:
        print "Skipping mutant", m
        continue
    while ((t >= 0) and (not foundCover)):
        print "Trying to find",t,"covering execution..."
        sys.stdout.flush()
        sys.stderr.flush()        
        cmd = "scbmc " + m + " " + harness + " " + options + "--find-success -DCOVTARGET=" + str(t)
        results = prefix + "." + m + ".bestcover"
        resultF = open (results, 'w')
        print cmd
        subprocess.call([cmd], shell=True, stdout = resultF, stderr = resultF)
        verOk = False
        for l in open (results):
            if "VERIFICATION SUCCESSFUL" in l:
                verOk = True
            if "VERIFICATION FAILED" in l:
                print "Successful!"
                sys.stdout.flush()
                sys.stderr.flush()
                foundCover = True
                break
        if not verOk:
            print "Failed to model check"
            t = -2
        if not foundCover:
            t -= 1
    if t < 0:
        print "Can't cover mutant at all!"
        sys.stdout.flush()
        sys.stderr.flush()
    mutStrength[m] = t

mstr = mutStrength.items()
mstrSort = sorted(mstr, key = lambda x: x[1], reverse=True)

print "STRENGTHS:"

for (m, str) in mstrSort:
    print str,":",m,
    n = 0
    for l in open(m):
        n += 1
        if "MUTANT" in l:
            print n,":",l,
    print
