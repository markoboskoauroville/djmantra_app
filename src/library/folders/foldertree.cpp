#include "library/folders/foldertree.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace djmantra {

namespace {

bool isInside(const QString& path, const QString& folder) {
    return path == folder || path.startsWith(folder + QChar('/'));
}

QString labelFor(const QString& path) {
    const QString name = QFileInfo(path).fileName();
    return name.isEmpty() ? path : name;
}

void insert(FolderNode* pNode, const QStringList& segments, int index, int count) {
    pNode->trackCount += count;
    if (index >= segments.size()) {
        return;
    }
    const QString childPath = pNode->path + QChar('/') + segments[index];
    auto it = std::find_if(pNode->children.begin(),
            pNode->children.end(),
            [&childPath](const FolderNode& child) { return child.path == childPath; });
    if (it == pNode->children.end()) {
        FolderNode child;
        child.path = childPath;
        child.label = segments[index];
        pNode->children.append(child);
        it = pNode->children.end() - 1;
    }
    insert(&*it, segments, index + 1, count);
}

void sortRecursively(QList<FolderNode>* pNodes) {
    std::sort(pNodes->begin(), pNodes->end(), [](const FolderNode& a, const FolderNode& b) {
        return a.label.compare(b.label, Qt::CaseInsensitive) < 0;
    });
    for (auto& node : *pNodes) {
        sortRecursively(&node.children);
    }
}

} // namespace

QList<FolderNode> buildFolderTree(const QStringList& roots,
        const QList<QPair<QString, int>>& trackDirs) {
    QList<FolderNode> tree;
    for (const auto& root : roots) {
        FolderNode node;
        node.path = QDir::cleanPath(root);
        node.label = labelFor(node.path);
        tree.append(node);
    }
    for (const auto& [directory, count] : trackDirs) {
        const QString dir = QDir::cleanPath(directory);
        // The innermost root that contains the folder (roots may be nested
        // after an upgrade of an old library)
        FolderNode* pRoot = nullptr;
        for (auto& node : tree) {
            if (isInside(dir, node.path) &&
                    (!pRoot || node.path.size() > pRoot->path.size())) {
                pRoot = &node;
            }
        }
        if (!pRoot) {
            continue; // a track outside the library folders
        }
        const QString relative = dir.mid(pRoot->path.size());
        insert(pRoot, relative.split(QChar('/'), Qt::SkipEmptyParts), 0, count);
    }
    sortRecursively(&tree);
    return tree;
}

QString folderCondition(const QString& directoryColumn, const QString& folder) {
    QString clean = QDir::cleanPath(folder);
    QString literal = clean;
    literal.replace(QChar('\''), QStringLiteral("''"));
    QString prefix = clean + QChar('/');
    prefix.replace(QChar('\''), QStringLiteral("''"));
    // substr() instead of LIKE: no escaping of % and _ needed
    return QStringLiteral("(%1='%2' OR substr(%1,1,%3)='%4')")
            .arg(directoryColumn, literal)
            .arg(clean.size() + 1)
            .arg(prefix);
}

} // namespace djmantra
