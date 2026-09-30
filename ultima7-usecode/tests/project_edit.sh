#!/bin/sh
# Builds a small project, changes it the way a person would, and builds it again:
# a routine keeps its number, a new one is numbered after it, calls and locals
# are kept right, and mistakes a person makes are refused.
# usage: project_edit.sh <u7ucproj> <u7ucdec> <scratch directory>
set -e
build=$1
decompile=$2
dir=$3
rm -rf "$dir"
mkdir -p "$dir/out"
fail() { echo "FAIL $1"; exit 1; }

cat > "$dir/project.u7p" <<'P'
u7project 1;
game "bg";
header "usecode.uh";
source_root ".";
link "usecode.lnk";
map "usecode.map";
P
cat > "$dir/usecode.uh" <<'H'
croutine SetSpeaker;
usableindex #Bench 100;
H
printf 'bench.use\ngreet.use\n' > "$dir/usecode.lnk"
cat > "$dir/bench.use" <<'U'
Usable Bench {
    if (event = 1) {
        Greet_i(item);
    }
}
U
cat > "$dir/greet.use" <<'U'
Routine Greet_i accepts a0 {
    v1 = v0;
    SetSpeaker(0, v1);
    ["Hello!"];
}
U

"$build" "$dir/project.u7p" "$dir/out" > /dev/null || fail "the project builds"
grep -q "^0800 Greet_i$" "$dir/usecode.map" || fail "the first build numbers from 0x800"
for f in USECODE LINKDEP1 LINKDEP2; do
    [ -s "$dir/out/$f" ] || fail "the build writes $f"
done

# A new routine placed first in build order still follows the ones already numbered.
cat > "$dir/bench.use" <<'U'
Routine Wave_i {
    v0 = 1;
}

Usable Bench {
    if (event = 1) {
        Greet_i(item);
        Wave_i();
    }
}
U
"$build" "$dir/project.u7p" "$dir/out" > /dev/null || fail "the changed project builds"
grep -q "^0800 Greet_i$" "$dir/usecode.map" || fail "an old routine keeps its number"
grep -q "^0801 Wave_i$" "$dir/usecode.map" || fail "a new routine is numbered after it"
"$decompile" --linear "$dir/out/USECODE" "$dir/out/listing.uc" > /dev/null
grep -q "Usable fn_0064 #locals(0) #uses(fn_0800, fn_0801)" "$dir/out/listing.uc" ||
    fail "the calls are in the caller's link table"
grep -q "fn_0801 #locals(1)" "$dir/out/listing.uc" || fail "the locals cover the slots used"

# Mistakes are refused, and say where.
sed 's/Wave_i();/Wave_i(5);/' "$dir/bench.use" > "$dir/wrong.use" && mv "$dir/wrong.use" "$dir/bench.use"
"$build" "$dir/project.u7p" "$dir/out" 2> "$dir/error.txt" > /dev/null && fail "a wrong argument count is refused"
grep -q "bench.use:8: Wave_i takes 0 arguments, not 1" "$dir/error.txt" || fail "the refusal says where"
sed 's/Wave_i(5);/v1 = Wave_i();/' "$dir/bench.use" > "$dir/wrong.use" && mv "$dir/wrong.use" "$dir/bench.use"
"$build" "$dir/project.u7p" "$dir/out" 2> "$dir/error.txt" > /dev/null && fail "using a value never returned is refused"
grep -q "Wave_i returns no value to use" "$dir/error.txt" || fail "that refusal says why"
sed 's/v1 = Wave_i();/Wave_i();/' "$dir/bench.use" > "$dir/wrong.use" && mv "$dir/wrong.use" "$dir/bench.use"

# The engine loads a function with everything it can call: 35 functions at most.
parts() {
    i=1
    while [ $i -le 35 ]; do
        printf 'Routine Part%d_i {\n    v0 = %d;\n}\n\n' $i $i
        i=$((i + 1))
    done
    printf 'Routine Whole_i {\n'
    i=1
    while [ $i -le "$1" ]; do
        printf '    Part%d_i();\n' $i
        i=$((i + 1))
    done
    printf '}\n'
}
parts 34 > "$dir/whole.use"
printf 'whole.use\n' >> "$dir/usecode.lnk"
"$build" "$dir/project.u7p" "$dir/out" > /dev/null || fail "35 functions at once build"
parts 35 > "$dir/whole.use"
rm "$dir/out/USECODE"
"$build" "$dir/project.u7p" "$dir/out" 2> "$dir/error.txt" > /dev/null && fail "36 functions at once are refused"
grep -q "whole.use:141: to run Whole_i the engine loads 36 functions" "$dir/error.txt" || fail "that refusal names the function"
[ ! -e "$dir/out/USECODE" ] || fail "a refused build writes nothing"
parts 34 > "$dir/whole.use"

# ... and 65,389 bytes of them.
text=$(head -c 33000 /dev/zero | tr '\0' 'a')
printf 'Routine Big1_i {\n    ["%s"];\n}\n\nRoutine Big2_i {\n    ["%s"];\n    Big1_i();\n}\n' \
    "$text" "$text" > "$dir/big.use"
printf 'big.use\n' >> "$dir/usecode.lnk"
"$build" "$dir/project.u7p" "$dir/out" 2> "$dir/error.txt" > /dev/null && fail "too many bytes at once are refused"
grep -q "to run Big2_i the engine loads 2 functions, 66037 bytes" "$dir/error.txt" || fail "that refusal gives the size"
echo "PASS project_edit"
