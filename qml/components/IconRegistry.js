.pragma library

function source(name) {
    if (!name || name.length === 0) {
        return ""
    }
    return Qt.resolvedUrl("../assets/icons/" + name + ".svg")
}
