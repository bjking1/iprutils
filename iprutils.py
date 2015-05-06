"""
Gather information about ipr adapters in a system and report it using plugins
supplied for application-specific information [improve this text]

"""
from sos.plugins import Plugin, IndependentPlugin

class IprPlugin(Plugin, IndependentPlugin):
	"""This plugin collects information about IPR adapters and other system information."""

	plugin_name = 'ipr-sos'
	profiles = ('hardware',)

	def setup(self):
		self.add_copy_spec(["/etc/ipr",
			"/var/log/iprdump*",
			"/sys/module/ipr"
		])
		self.add_cmd_output([
			"/usr/sbin/iprconfig -c dump"
		])
