git rev-parse --git-dir >/dev/null || exit 1
git log -1 --format=format:%ci%n | sed -e 's/ [-+].*$//;s/ /T/;s/^/D /' > manifest
echo $(git log -1 --format=format:%H) > manifest.uuid
