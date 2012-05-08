#include "StatusJob.h"
#include "Server.h"
#include "RTags.h"
#include "Indexer.h"
#include <clang-c/Index.h>
#include <Rdm.h>
#include "CursorInfo.h"

StatusJob::StatusJob(int i, const QByteArray &q)
    : Job(i, WriteUnfiltered), query(q)
{
}

void StatusJob::execute()
{
    if (query.isEmpty() || query == "general") {
        Database *db = Server::instance()->db(Server::General);
        write(Server::databaseDir(Server::General));
        write("    version: " + QByteArray::number(db->value<int>("version")));
    }

    if (query.isEmpty() || query == "dependencies") {
        Database *db = Server::instance()->db(Server::Dependency);
        write(Server::databaseDir(Server::Dependency));
        RTags::Ptr<Iterator> it(db->createIterator());
        it->seekToFirst();
        char buf[1024];
        memcpy(buf, "  ", 2);
        while (it->isValid()) {
            if (isAborted())
                return;
            memcpy(buf + 2, it->key().data(), it->key().size());
            memcpy(buf + 2 + it->key().size(), " is depended on by:", 20);
            write(buf);
            const QSet<Path> deps = it->value<QSet<Path> >();
            memcpy(buf + 2, "  ", 2);
            foreach (const Path &p, deps) {
                memcpy(buf + 4, p.constData(), p.size() + 1);
                write(buf);
            }
            it->next();
        }
    }

    if (query.isEmpty() || query == "symbols") {
        Database *db = Server::instance()->db(Server::Symbol);
        write(Server::databaseDir(Server::Symbol));
        RTags::Ptr<Iterator> it(db->createIterator());
        it->seekToFirst();
        char buf[1024];
        memcpy(buf, "  ", 2);
        while (it->isValid()) {
            if (isAborted())
                return;
            memcpy(buf + 2, it->key().data(), it->key().size());
            const CursorInfo ci = it->value<CursorInfo>();
            CXString kind = clang_getCursorKindSpelling(ci.kind);
            snprintf(buf + 2 + it->key().size(), sizeof(buf) - it->key().size() - 3,
                     " kind: %s symbolLength: %d symbolName: %s target: %s%s",
                     clang_getCString(kind), ci.symbolLength, ci.symbolName.constData(),
                     ci.target.key(Location::Padded).constData(),
                     ci.references.isEmpty() ? "" : " references:");
            clang_disposeString(kind);
            write(buf);
            foreach(const Location &loc, ci.references) {
                const int w = snprintf(buf + 2, sizeof(buf) - 4, "  %s",
                                       loc.key(Location::Padded).constData());
                write(QByteArray(buf, w + 2));
            }
            it->next();
        }
    }

    if (query.isEmpty() || query == "symbolnames") {
        Database *db = Server::instance()->db(Server::SymbolName);
        write(Server::databaseDir(Server::SymbolName));
        RTags::Ptr<Iterator> it(db->createIterator());
        it->seekToFirst();
        char buf[1024];
        memcpy(buf, "  ", 2);
        while (it->isValid()) {
            if (isAborted())
                return;
            memcpy(buf + 2, it->key().data(), it->key().size());
            memcpy(buf + 2 + it->key().size(), ":", 2);
            write(buf);
            const QSet<Location> locations = it->value<QSet<Location> >();
            memcpy(buf + 2, "  ", 2);
            foreach (const Location &loc, locations) {
                QByteArray key = loc.key(Location::Padded);
                memcpy(buf + 4, key.constData(), key.size() + 1);
                write(buf);
            }
            it->next();
        }
    }

    if (query.isEmpty() || query == "fileinfos") {
        Database *db = Server::instance()->db(Server::FileInformation);
        write(Server::databaseDir(Server::FileInformation));
        RTags::Ptr<Iterator> it(db->createIterator());
        it->seekToFirst();
        char buf[1024];
        memcpy(buf, "  ", 2);
        while (it->isValid()) {
            if (isAborted())
                return;
            memcpy(buf + 2, it->key().data(), it->key().size());
            const FileInformation fi = it->value<FileInformation>();
            snprintf(buf + 2 + it->key().size(), sizeof(buf) - 3 - it->key().size(),
                     ": %s [%s]", QDateTime::fromTime_t(fi.lastTouched).toString().toLocal8Bit().constData(),
                     RTags::join(fi.compileArgs).constData());
            write(buf);
            it->next();
        }
    }

    if (query.isEmpty() || query == "pch") {
        // ### needs to be done
    }

    if (query.isEmpty() || query == "fileids") {
        Database *db = Server::instance()->db(Server::FileIds);
        write(Server::databaseDir(Server::FileIds));
        RTags::Ptr<Iterator> it(db->createIterator());
        it->seekToFirst();
        char buf[1024];
        while (it->isValid()) {
            snprintf(buf, 1024, "  %s: %d", std::string(it->key().data(), it->key().size()).c_str(), it->value<quint32>());
            it->next();
        }
    }
}
