// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_COMMAND_COMMAND_HPP_
#define _SIRIKATA_LIBCORE_COMMAND_COMMAND_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <boost/property_tree/ptree.hpp>

namespace Sirikata {

/** The Command namespace contains classes for handling external command
 *  requests, allowing external tools to interact with a live Sirikata process.
 */
namespace Command {

class Commander;

/** A CommandKey is a name for a command. Commands always have a top-level
 * 'command' entry which will be set to this value when you construct one. You
 * should use a hierarchy when naming commands to avoid conflicts, e.g. rather
 * than 'stats', use 'space.forwarder.stats'.
 */
typedef String CommandKey;

/** Unique ID for a command request. These are passed to the CommandHandler so
 *  it can later return the result.
 */
typedef uint32 CommandID;

// We'd prefer a class that inherits from ptree and adds constraints, but
// boost::property_tree functions don't seem to handle this well.
/** Commands take the form of property trees. These are pretty generic
 *  tree-structured values, making commands pretty flexible.
 */
typedef boost::property_tree::ptree Command;
bool SIRIKATA_FUNCTION_EXPORT CommandIsValid(const Command& cmd);
void SIRIKATA_FUNCTION_EXPORT CommandSetName(Command& cmd, const String& name);


/** Results are returned by executing a command. These are also generic
 *  tree-structured values, allowing complex data to be returned without
 *  Commandables needing to worry about encoding.
 */
typedef boost::property_tree::ptree Result;


/** CommandHandlers actually process Commands and return a Result to be
 *  forwarded to the requestor. Instead of using a return value, the
 *  CommandHandler should invoke Commander::result() using the Commander and
 *  CommandID passed to it. This allows CommandHandlers to operate
 *  asynchronously (e.g. just to get on the right thread/strand or because a
 *  command requires asynchronous steps).
 */
typedef std::tr1::function<void(const Command&, Commander*, CommandID)> CommandHandler;



} // namespace Command
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_COMMAND_COMMAND_HPP_
